// Windows Event Sender - YAML direct integration
// Provides a helper to read monitored directory paths from fim_config.yml using yaml-cpp only

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <cctype>

#include <yaml-cpp/yaml.h>

#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")
#include <iostream>

namespace fim {

// Returns the list of directory paths to monitor where the entry is enabled (or enabled missing -> true).
// Throws std::runtime_error if the config cannot be read/parsed.
std::vector<std::string> get_monitored_paths(const std::string& config_path) {
	YAML::Node root = YAML::LoadFile(config_path);
	if (!root || !root.IsMap()) {
		throw std::runtime_error("FIM config root must be a map/object");
	}

	std::vector<std::string> paths;
	const YAML::Node md = root["monitored_directories"];
	if (!md || !md.IsSequence()) {
		// No monitored directories is not fatal; return empty list
		return paths;
	}

	for (const auto& item : md) {
		if (!item || !item.IsMap()) continue;
		// enabled defaults to true if missing
		bool enabled = true;
		if (const auto en = item["enabled"]) {
			try { enabled = en.as<bool>(); } catch (...) { /* keep default */ }
		}
		if (!enabled) continue;

		if (const auto p = item["path"]) {
			try {
				auto path = p.as<std::string>();
				if (!path.empty()) paths.emplace_back(std::move(path));
			} catch (...) {
				// skip malformed path
			}
		}
	}
	return paths;
}

} // namespace fim

// -------------- Windows Event Log (Winevtapi) subscription scaffolding --------------

namespace {

static std::string trim_whitespace(const std::string& input) {
	size_t start = 0;
	while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
		++start;
	}
	size_t end = input.size();
	while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
		--end;
	}
	return input.substr(start, end - start);
}

static std::string path_to_utf8(const std::filesystem::path& p) {
#if defined(_WIN32)
	auto u8 = p.u8string();
	return std::string(u8.begin(), u8.end());
#else
	return p.string();
#endif
}

static bool set_env_variable(const std::string& key, const std::string& value) {
#if defined(_WIN32)
	return _putenv_s(key.c_str(), value.c_str()) == 0;
#else
	return setenv(key.c_str(), value.c_str(), 1) == 0;
#endif
}

// Helper: UTF-8 to UTF-16
static std::wstring to_wstring(const std::string& s) {
	if (s.empty()) return std::wstring();
	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	std::wstring ws;
	ws.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), ws.data(), len);
	return ws;
}

// Helper: UTF-16 to UTF-8
static std::string to_utf8(const std::wstring& ws) {
    if (ws.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    std::string s;
    s.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), s.data(), len, nullptr, nullptr);
    return s;
}

// Case-insensitive starts-with for Windows paths
static bool starts_with_path_icase(const std::wstring& path, const std::wstring& prefix) {
	if (prefix.empty()) return true;
	if (path.size() < prefix.size()) return false;
	return _wcsnicmp(path.c_str(), prefix.c_str(), prefix.size()) == 0;
}

static std::string render_event_xml_utf8(EVT_HANDLE event) {
	DWORD bufferUsed = 0;
	DWORD propertyCount = 0;
	if (!EvtRender(nullptr, event, EvtRenderEventXml, 0, nullptr, &bufferUsed, &propertyCount)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			return std::string();
		}
	}
	if (bufferUsed == 0) return std::string();
	std::vector<WCHAR> buffer(bufferUsed / sizeof(WCHAR));
	if (!EvtRender(nullptr, event, EvtRenderEventXml, bufferUsed, buffer.data(), &bufferUsed, &propertyCount)) {
		return std::string();
	}
	return to_utf8(std::wstring(buffer.data()));
}

struct SubscriptionCtx {
	std::vector<std::wstring> prefixes; // monitored directory prefixes
};

// Try extract a file path from an event using known fields for Sysmon and Security
static std::wstring extract_path_from_event(EVT_HANDLE event, SubscriptionCtx* ctx) {
	// We’ll try, in order, common fields:
	// - Sysmon: Event/EventData/Data[@Name='TargetFilename']
	// - Security 4663: Event/EventData/Data[@Name='ObjectName']
	PCWSTR valuePaths[2] = {
		L"Event/EventData/Data[@Name='TargetFilename']",
		L"Event/EventData/Data[@Name='ObjectName']",
	};

	for (PCWSTR vp : valuePaths) {
		EVT_HANDLE localCtx = EvtCreateRenderContext(1, &vp, EvtRenderContextValues);
		if (!localCtx) continue;

		DWORD bufSize = 0, propCount = 0;
		(void)EvtRender(localCtx, event, EvtRenderEventValues, 0, nullptr, &bufSize, &propCount);
		std::vector<BYTE> buf(bufSize);
		if (EvtRender(localCtx, event, EvtRenderEventValues, (DWORD)buf.size(), buf.data(), &bufSize, &propCount)) {
			auto props = reinterpret_cast<PEVT_VARIANT>(buf.data());
			if (propCount >= 1) {
				if (props[0].Type == EvtVarTypeString && props[0].StringVal) {
					std::wstring val(props[0].StringVal);
					EvtClose(localCtx);
					return val;
				}
			}
		}
		EvtClose(localCtx);
	}
	return std::wstring();
}

// Extract EventID from the event's system properties
static USHORT get_event_id(EVT_HANDLE event) {
	USHORT id = 0;
	EVT_HANDLE sysCtx = EvtCreateRenderContext(0, nullptr, EvtRenderContextSystem);
	if (!sysCtx) return id;
	DWORD bufSize = 0, propCount = 0;
	(void)EvtRender(sysCtx, event, EvtRenderEventValues, 0, nullptr, &bufSize, &propCount);
	std::vector<BYTE> buf(bufSize);
	if (EvtRender(sysCtx, event, EvtRenderEventValues, (DWORD)buf.size(), buf.data(), &bufSize, &propCount)) {
		auto props = reinterpret_cast<PEVT_VARIANT>(buf.data());
		// EVTSYSTEM_PROPERTY_ID enum index for EventID
		// See winevt.h: EvtSystemEventID
		const size_t EvtSystemEventID_Index = 2; // typical index for EventID
		if (propCount > EvtSystemEventID_Index) {
			id = props[EvtSystemEventID_Index].UInt16Val;
		}
	}
	EvtClose(sysCtx);
	return id;
}

static std::string build_event_object_suffix(USHORT eventId) {
	SYSTEMTIME st;
	GetSystemTime(&st);
	std::ostringstream oss;
	oss << std::setfill('0')
		<< std::setw(4) << st.wYear
		<< std::setw(2) << st.wMonth
		<< std::setw(2) << st.wDay
		<< 'T'
		<< std::setw(2) << st.wHour
		<< std::setw(2) << st.wMinute
		<< std::setw(2) << st.wSecond
		<< '.' << std::setw(3) << st.wMilliseconds
		<< 'Z'
		<< "_evt-" << eventId
		<< "_pid-" << GetCurrentProcessId()
		<< ".xml";
	return oss.str();
}

// Subscription callback
static DWORD WINAPI evt_callback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID userCtx, EVT_HANDLE event) {
	auto* ctx = reinterpret_cast<SubscriptionCtx*>(userCtx);
	switch (action) {
		case EvtSubscribeActionError: {
			// Could log GetLastError(); ignoring here
			break;
		}
		case EvtSubscribeActionDeliver: {
			std::wstring target = extract_path_from_event(event, ctx);
			if (!target.empty()) {
				USHORT evId = get_event_id(event);
				const wchar_t* label = L"EVENT";
				// Common Sysmon file EventIDs; Security 4663 = access attempt
				if (evId == 11) label = L"CREATED";
				else if (evId == 23) label = L"DELETED";
				else if (evId == 26) label = L"DELETE_DETECTED";
				else if (evId == 4663) label = L"ACCESS";
				for (const auto& pref : ctx->prefixes) {
					if (starts_with_path_icase(target, pref)) {
						// Print locally and forward to remote storage if configured.
						DWORD pid = GetCurrentProcessId();
						fwprintf(stdout, L"[PID %lu] %s : %s\n", pid, label, target.c_str());
						fflush(stdout);
						break;
					}
				}
			}
			break;
		}
		default: break;
	}
	return 0; // continue
}

// Start subscription to Sysmon (and optionally Security) channels
static EVT_HANDLE start_sysmon_subscription(SubscriptionCtx* ctx) {
	// Subscribe to common Sysmon file events: 11 (FileCreate), 23 (FileDelete), 26 (FileDeleteDetected),
	// 12 (RegistryEvent - not file), 13 (RegistryValueSet) included above only as example; keep file-related ones.
	// We'll use 11, 23, 26 for file ops.
	PCWSTR channel = L"Microsoft-Windows-Sysmon/Operational";
	PCWSTR query = L"*[System[(EventID=11 or EventID=23 or EventID=26)]]";
	EVT_HANDLE sub = EvtSubscribe(
		nullptr,
		nullptr,
		channel,
		query,
		nullptr,
		ctx,
		evt_callback,
		EvtSubscribeToFutureEvents // future events only; avoids invalid flag combos
	);
	if (!sub) {
		DWORD err = GetLastError();
		fwprintf(stderr, L"Sysmon subscribe failed. GetLastError=%lu\n", err);
	}
	return sub;
}

static EVT_HANDLE start_security_subscription(SubscriptionCtx* ctx) {
	// Security 4663 (An attempt was made to access an object). Requires Object Access auditing + SACLs on the folders.
	PCWSTR channel = L"Security";
	PCWSTR query = L"*[System[(EventID=4663)]]";
	EVT_HANDLE sub = EvtSubscribe(
		nullptr,
		nullptr,
		channel,
		query,
		nullptr,
		ctx,
		evt_callback,
		EvtSubscribeToFutureEvents
	);
	if (!sub) {
		DWORD err = GetLastError();
		fwprintf(stderr, L"Security subscribe failed. GetLastError=%lu\n", err);
	}
	return sub;
}

} // anonymous namespace

int wmain(int argc, wchar_t** wargv) {

	std::wstring cfg = argc > 1 ? wargv[1] : L"fim_config.yml";
	// Load prefixes from YAML (UTF-8 file path assumed)
	std::vector<std::string> u8paths;
	try {
		u8paths = fim::get_monitored_paths(to_utf8(cfg));
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	SubscriptionCtx ctx;
	ctx.prefixes.reserve(u8paths.size());
	for (const auto& p : u8paths) {
		// Normalize to backslash-prefixed Windows path; allow both C:\ and \\?\ forms
		auto ws = to_wstring(p);
		ctx.prefixes.push_back(ws);
	}

	EVT_HANDLE sysmonSub = start_sysmon_subscription(&ctx);
	if (!sysmonSub) {
		std::cerr << "Failed to subscribe to Sysmon channel. Ensure Sysmon is installed and the Operational log is enabled." << std::endl;
	}

	EVT_HANDLE secSub = start_security_subscription(&ctx); // may be nullptr if no permission/auditing
	if (!secSub) {
		std::cerr << "Security subscription may not be active (requires audit policy and SACLs)." << std::endl;
	}

	std::wcout << L"Event subscriptions active. Press Ctrl+C to exit." << std::endl;
	// Simple wait loop
	while (true) Sleep(1000);

	// Cleanup (unreachable here, but good practice if you adapt)
	if (sysmonSub) EvtClose(sysmonSub);
	if (secSub) EvtClose(secSub);
	return 0;
}
