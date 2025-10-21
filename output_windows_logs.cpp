#include <windows.h>
#include <winevt.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "wevtapi.lib")
// Requires the windows SDK to compile, what it's doing is printing out all windows event logs


static void PrintLastError(const char* msg) {
    DWORD err = GetLastError();
    std::cerr << msg << " (error " << err << ")\n";
}

int wmain()
{
    // Enumerate channels
    EVT_HANDLE hChannels = EvtOpenChannelEnum(NULL, 0);
    if (!hChannels) {
        PrintLastError("EvtOpenChannelEnum failed");
        return 1;
    }

    std::vector<wchar_t> channelBuf(512);
    DWORD channelUsed = 0;

    while (true) {
        if (!EvtNextChannelPath(hChannels, (DWORD)channelBuf.size(), channelBuf.data(), &channelUsed)) {
            DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_ITEMS) break;
            if (err == ERROR_INSUFFICIENT_BUFFER) {
                channelBuf.resize(channelUsed);
                if (!EvtNextChannelPath(hChannels, (DWORD)channelBuf.size(), channelBuf.data(), &channelUsed)) {
                    PrintLastError("EvtNextChannelPath retry failed");
                    break;
                }
            } else {
                PrintLastError("EvtNextChannelPath failed");
                break;
            }
        }

        std::wstring channelName(channelBuf.data());
        std::wcout << L"=== Channel: " << channelName << L" ===\n";

        // Query all events from the channel
        EVT_HANDLE hQuery = EvtQuery(NULL, channelName.c_str(), NULL, EvtQueryChannelPath);
        if (!hQuery) {
            PrintLastError("EvtQuery failed");
            continue;
        }

        const DWORD batchSize = 16;
        std::vector<EVT_HANDLE> events(batchSize);
        DWORD returned = 0;

        while (EvtNext(hQuery, batchSize, events.data(), INFINITE, 0, &returned)) {
            for (DWORD i = 0; i < returned; ++i) {
                // Render event as XML
                DWORD bufferSize = 0;
                DWORD propertyCount = 0;
                if (!EvtRender(NULL, events[i], EvtRenderEventXml, 0, NULL, &bufferSize, &propertyCount)) {
                    DWORD err = GetLastError();
                    if (err == ERROR_INSUFFICIENT_BUFFER) {
                        std::vector<wchar_t> renderBuf(bufferSize / sizeof(wchar_t));
                        if (EvtRender(NULL, events[i], EvtRenderEventXml, bufferSize, renderBuf.data(), &bufferSize, &propertyCount)) {
                            // Print XML (wide)
                            std::wcout << renderBuf.data() << L"\n\n";
                        } else {
                            PrintLastError("EvtRender failed");
                        }
                    } else {
                        PrintLastError("EvtRender failed (no buffer)");
                    }
                }
                EvtClose(events[i]);
            }
        }

        DWORD last = GetLastError();
        if (last != ERROR_NO_MORE_ITEMS && last != ERROR_SUCCESS) {
            PrintLastError("EvtNext (events) failed");
        }

        EvtClose(hQuery);
    }

    EvtClose(hChannels);
    return 0;
}