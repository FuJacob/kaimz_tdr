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
#include <cwctype>
#include <cstdio>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
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

static bool load_env_file(const std::filesystem::path& envPath) {
	std::error_code ec;
	if (!std::filesystem::exists(envPath, ec)) {
		return false;
	}
	std::ifstream file(envPath);
	if (!file.is_open()) {
		std::cerr << "[FIM] Unable to open env file: " << path_to_utf8(envPath) << std::endl;
		return false;
	}

	bool anyApplied = false;
	std::string line;
	size_t lineNo = 0;
	while (std::getline(file, line)) {
		++lineNo;
		std::string trimmed = trim_whitespace(line);
		if (trimmed.empty() || trimmed[0] == '#') continue;
		const auto pos = trimmed.find('=');
		if (pos == std::string::npos) {
			std::cerr << "[FIM] Skipping malformed .env line " << lineNo << std::endl;
			continue;
		}
		std::string key = trim_whitespace(trimmed.substr(0, pos));
		std::string value = trim_whitespace(trimmed.substr(pos + 1));
		if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
		                          (value.front() == '\'' && value.back() == '\''))) {
			value = value.substr(1, value.size() - 2);
		}
		if (key.empty()) continue;
		if (!set_env_variable(key, value)) {
			std::cerr << "[FIM] Failed to set env var '" << key << "' from " << path_to_utf8(envPath) << std::endl;
			continue;
		}
		anyApplied = true;
	}

	if (anyApplied) {
		std::cout << "[FIM] Loaded environment variables from " << path_to_utf8(envPath) << std::endl;
	}
	return anyApplied;
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

static std::string json_escape(const std::string& input) {
	std::string out;
	out.reserve(input.size() + 16);
	for (unsigned char c : input) {
		switch (c) {
		case '\\': out += "\\\\"; break;
		case '\"': out += "\\\""; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (c < 0x20) {
				char buf[7];
				std::snprintf(buf, sizeof(buf), "\\u%04x", c);
				out.append(buf, 6);
			} else {
				out.push_back(static_cast<char>(c));
			}
		}
	}
	return out;
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

// Minimal HTTP uploader that forwards rendered XML blobs to the configured API endpoint.
// Requires FIM_API_URL (and optional FIM_API_TOKEN) environment variables.
class ApiUploader {
public:
	ApiUploader() = default;

	void refresh_from_env() {
		std::lock_guard<std::mutex> lock(uploadMutex_);
		endpoint_ = getenv_string("FIM_API_URL");
		token_ = getenv_string("FIM_API_TOKEN");
	}

	bool configured() const {
		std::lock_guard<std::mutex> lock(uploadMutex_);
		return !endpoint_.empty();
	}

		bool upload_payload(const std::string& keySuffix, const std::string& payload) {
			if (payload.empty()) return false;
			std::lock_guard<std::mutex> lock(uploadMutex_);
			if (endpoint_.empty()) return false;
			return send_locked(keySuffix, payload);
		}

	ApiUploader(const ApiUploader&) = delete;
	ApiUploader& operator=(const ApiUploader&) = delete;

private:
	struct ParsedUrl {
		std::wstring host;
		std::wstring resource;
		INTERNET_PORT port{0};
		bool secure{false};
	};

		bool send_locked(const std::string& keySuffix, const std::string& payload) {
			ParsedUrl url;
			if (!parse_url_locked(url)) {
				std::cerr << "[FIM] Invalid FIM_API_URL, unable to forward event." << std::endl;
			return false;
		}

		HINTERNET session = WinHttpOpen(L"FIM/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!session) {
			std::cerr << "[FIM] WinHttpOpen failed: " << GetLastError() << std::endl;
			return false;
		}

		HINTERNET connect = WinHttpConnect(session, url.host.c_str(), url.port, 0);
		if (!connect) {
			std::cerr << "[FIM] WinHttpConnect failed: " << GetLastError() << std::endl;
			WinHttpCloseHandle(session);
			return false;
		}

		DWORD flags = url.secure ? WINHTTP_FLAG_SECURE : 0;
		HINTERNET request = WinHttpOpenRequest(connect, L"POST", url.resource.c_str(),
			nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
		if (!request) {
			std::cerr << "[FIM] WinHttpOpenRequest failed: " << GetLastError() << std::endl;
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			return false;
		}

		std::wstring headers = L"Content-Type: application/json; charset=utf-8\r\n";
		if (!token_.empty()) {
			headers += L"Authorization: Bearer " + to_wstring(token_) + L"\r\n";
		}

		std::string body = "{\"log\":\"" + json_escape(payload) + "\",\"filename\":\"" + json_escape(keySuffix) + "\"}";
		DWORD bodySize = static_cast<DWORD>(body.size());
		LPVOID bodyPtr = body.empty() ? nullptr : const_cast<char*>(body.data());

		BOOL sent = WinHttpSendRequest(
			request,
			headers.c_str(),
			(DWORD)-1,
			bodyPtr,
			bodySize,
			bodySize,
			0);
		if (!sent) {
			std::cerr << "[FIM] WinHttpSendRequest failed: " << GetLastError() << std::endl;
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			return false;
		}

		if (!WinHttpReceiveResponse(request, nullptr)) {
			std::cerr << "[FIM] WinHttpReceiveResponse failed: " << GetLastError() << std::endl;
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			return false;
		}

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		return true;
	}

	bool parse_url_locked(ParsedUrl& parsed) const {
		if (endpoint_.empty()) return false;
		std::wstring wideUrl = to_wstring(endpoint_);
		URL_COMPONENTS comps{};
		ZeroMemory(&comps, sizeof(comps));
		comps.dwStructSize = sizeof(comps);
		comps.dwSchemeLength = (DWORD)-1;
		comps.dwHostNameLength = (DWORD)-1;
		comps.dwUrlPathLength = (DWORD)-1;
		comps.dwExtraInfoLength = (DWORD)-1;

		if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &comps)) {
			std::cerr << "[FIM] WinHttpCrackUrl failed: " << GetLastError() << std::endl;
			return false;
		}

		if (!comps.lpszHostName || comps.dwHostNameLength == 0) {
			return false;
		}

		parsed.secure = (comps.nScheme == INTERNET_SCHEME_HTTPS);
		parsed.port = comps.nPort ? comps.nPort :
			(parsed.secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);
		parsed.host.assign(comps.lpszHostName, comps.dwHostNameLength);
		parsed.resource.clear();
		if (comps.dwUrlPathLength > 0 && comps.lpszUrlPath) {
			parsed.resource.assign(comps.lpszUrlPath, comps.dwUrlPathLength);
		}
		if (parsed.resource.empty()) {
			parsed.resource = L"/";
		}
		if (comps.dwExtraInfoLength > 0 && comps.lpszExtraInfo) {
			parsed.resource.append(comps.lpszExtraInfo, comps.dwExtraInfoLength);
		}
		return true;
	}

	static std::string getenv_string(const char* name) {
		if (const char* value = std::getenv(name)) return value;
		return {};
	}

	std::string endpoint_;
	std::string token_;
	mutable std::mutex uploadMutex_;
};

static ApiUploader g_api_uploader;

static void maybe_send_event_to_api(EVT_HANDLE event, USHORT eventId) {
	static std::once_flag warnOnce;
	if (!g_api_uploader.configured()) {
		std::call_once(warnOnce, []() {
			std::cerr << "[FIM] Remote uploads disabled. Provide FIM_API_URL (and optional FIM_API_TOKEN) "
			          << "to forward events to the backend API." << std::endl;
		});
		return;
	}
	std::string xml = render_event_xml_utf8(event);
	if (xml.empty()) return;
	const std::string keySuffix = build_event_object_suffix(eventId);
	if (!g_api_uploader.upload_payload(keySuffix, xml)) {
		std::cerr << "[FIM] Failed to POST Windows Event XML to API (object=" << keySuffix << ")." << std::endl;
	}
}

// -------------- File hash tracking and change reporting --------------

struct FileHashInfo {
	std::wstring originalPath;
	std::wstring fileName;
	std::string hashHex;
};

enum class HashLogMode {
	Silent,
	Verbose,
};

static std::unordered_map<std::wstring, FileHashInfo> g_file_hashes;
static std::mutex g_file_hash_mutex;

static std::wstring normalize_path_key(const std::wstring& path) {
	std::wstring normalized = path;
	for (auto& ch : normalized) {
		if (ch == L'/') ch = L'\\';
		ch = static_cast<wchar_t>(std::towlower(ch));
	}
	return normalized;
}

static std::wstring extract_filename(const std::wstring& fullPath) {
	try {
		const std::filesystem::path fsPath(fullPath);
		std::wstring filename = fsPath.filename().wstring();
		if (!filename.empty()) return filename;
	} catch (...) {
	}
	return fullPath;
}

static std::string bytes_to_hex(const std::vector<BYTE>& data) {
	static const char* digits = "0123456789abcdef";
	std::string hex;
	hex.resize(data.size() * 2);
	for (size_t i = 0; i < data.size(); ++i) {
		hex[2 * i] = digits[(data[i] >> 4) & 0xF];
		hex[2 * i + 1] = digits[data[i] & 0xF];
	}
	return hex;
}

static bool compute_file_sha256(const std::wstring& filePath, std::string& hashHex) {
	HANDLE file = CreateFileW(
		filePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		return false;
	}

	BCRYPT_ALG_HANDLE alg = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	std::vector<BYTE> hashObject;
	std::vector<BYTE> hashBuffer;
	bool success = false;
	DWORD objectLength = 0;
	DWORD hashLength = 0;
	ULONG cbData = 0;

	NTSTATUS status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
	if (!BCRYPT_SUCCESS(status)) goto cleanup;

	status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &cbData, 0);
	if (!BCRYPT_SUCCESS(status)) goto cleanup;

	status = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &cbData, 0);
	if (!BCRYPT_SUCCESS(status)) goto cleanup;

	hashObject.resize(objectLength);
	hashBuffer.resize(hashLength);

	status = BCryptCreateHash(alg, &hash, hashObject.data(), objectLength, nullptr, 0, 0);
	if (!BCRYPT_SUCCESS(status)) goto cleanup;

	{
		std::vector<BYTE> buffer(64 * 1024);
		DWORD bytesRead = 0;
		while (true) {
			if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)) {
				const DWORD err = GetLastError();
				if (err == ERROR_HANDLE_EOF) break;
				goto cleanup;
			}
			if (bytesRead == 0) break;
			status = BCryptHashData(hash, buffer.data(), bytesRead, 0);
			if (!BCRYPT_SUCCESS(status)) goto cleanup;
		}
	}

	status = BCryptFinishHash(hash, hashBuffer.data(), static_cast<ULONG>(hashBuffer.size()), 0);
	if (!BCRYPT_SUCCESS(status)) goto cleanup;

	hashHex = bytes_to_hex(hashBuffer);
	success = true;

cleanup:
	if (hash) BCryptDestroyHash(hash);
	if (alg) BCryptCloseAlgorithmProvider(alg, 0);
	CloseHandle(file);
	return success;
}

static std::string build_hash_log_suffix(const char* tag) {
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
		<< "_hash-" << tag
		<< "_pid-" << GetCurrentProcessId()
		<< ".log";
	return oss.str();
}

static void emit_hash_log_entry(const std::wstring& prefix, const std::wstring& path, const std::string& previousHash, const std::string& newHash, const char* tag) {
	std::wstringstream wss;
	wss << L"[HASH] " << prefix << L" path=" << path;
	if (!previousHash.empty()) {
		wss << L" previous=" << to_wstring(previousHash);
	}
	if (!newHash.empty()) {
		wss << L" current=" << to_wstring(newHash);
	}
	const std::wstring line = wss.str();
	std::wcout << line << std::endl;

	if (g_api_uploader.configured()) {
		g_api_uploader.upload_payload(build_hash_log_suffix(tag), to_utf8(line));
	}
}

static void upsert_hash_record(const std::wstring& fullPath, const std::string& newHash, HashLogMode mode) {
	if (newHash.empty()) return;

	const std::wstring key = normalize_path_key(fullPath);
	FileHashInfo record;
	record.originalPath = fullPath;
	record.fileName = extract_filename(fullPath);
	record.hashHex = newHash;

	bool isNew = false;
	bool changed = false;
	std::string previousHash;

	{
		std::lock_guard<std::mutex> lock(g_file_hash_mutex);
		auto it = g_file_hashes.find(key);
		if (it == g_file_hashes.end()) {
			g_file_hashes.emplace(key, record);
			isNew = true;
		} else if (it->second.hashHex != newHash) {
			previousHash = it->second.hashHex;
			it->second = record;
			changed = true;
		} else {
			return; // unchanged
		}
	}

	if (mode == HashLogMode::Verbose) {
		if (isNew) {
			emit_hash_log_entry(L"Recorded baseline hash", fullPath, {}, newHash, "add");
		} else if (changed) {
			emit_hash_log_entry(L"Hash changed", fullPath, previousHash, newHash, "change");
		}
	}
}

static void remove_hash_record(const std::wstring& fullPath, HashLogMode mode) {
	const std::wstring key = normalize_path_key(fullPath);
	FileHashInfo removed;
	bool existed = false;

	{
		std::lock_guard<std::mutex> lock(g_file_hash_mutex);
		auto it = g_file_hashes.find(key);
		if (it != g_file_hashes.end()) {
			removed = it->second;
			g_file_hashes.erase(it);
			existed = true;
		}
	}

	if (existed && mode == HashLogMode::Verbose) {
		emit_hash_log_entry(L"Hash entry removed", fullPath, removed.hashHex, {}, "remove");
	}
}

static void index_existing_file(const std::wstring& filePath, HashLogMode mode) {
	std::string hash;
	if (compute_file_sha256(filePath, hash)) {
		upsert_hash_record(filePath, hash, mode);
	}
}

static void build_initial_hash_index(const std::vector<std::wstring>& rootPaths) {
	for (const auto& root : rootPaths) {
		std::error_code ec;
		const std::filesystem::path fsPath(root);
		if (!std::filesystem::exists(fsPath, ec)) {
			std::wcerr << L"[HASH] Skipping missing path: " << root << std::endl;
			continue;
		}

		if (std::filesystem::is_regular_file(fsPath, ec)) {
			index_existing_file(fsPath.wstring(), HashLogMode::Silent);
			continue;
		}

		if (!std::filesystem::is_directory(fsPath, ec)) {
			continue;
		}

		try {
			const std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied;
			for (std::filesystem::recursive_directory_iterator it(fsPath, opts), end; it != end; ++it) {
				std::error_code entryEc;
				if (it->is_directory(entryEc)) {
					continue;
				}
				if (it->is_regular_file(entryEc)) {
					index_existing_file(it->path().wstring(), HashLogMode::Silent);
				}
			}
		} catch (const std::exception& ex) {
			std::wcerr << L"[HASH] Failed to index " << root << L": " << to_wstring(ex.what()) << std::endl;
		}
	}
}

static void handle_hash_tracking_for_event(const std::wstring& fullPath, USHORT eventId) {
	if (eventId == 23 || eventId == 26) {
		remove_hash_record(fullPath, HashLogMode::Verbose);
		return;
	}

	std::string newHash;
	if (!compute_file_sha256(fullPath, newHash)) {
		std::wcerr << L"[HASH] Unable to compute hash for " << fullPath << std::endl;
		return;
	}
	upsert_hash_record(fullPath, newHash, HashLogMode::Verbose);
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
							maybe_send_event_to_api(event, evId);
							handle_hash_tracking_for_event(target, evId);
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

// -------------- Production entrypoint for Winevtapi integration --------------
int wmain(int argc, wchar_t** wargv) {
	std::filesystem::path envPath = ".env";
	if (const char* overridePath = std::getenv("FIM_ENV_FILE")) {
		envPath = overridePath;
	}
	load_env_file(envPath);
	g_api_uploader.refresh_from_env();

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
	build_initial_hash_index(ctx.prefixes);

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
