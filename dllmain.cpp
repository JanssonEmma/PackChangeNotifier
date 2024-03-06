#include "pch.h"
#include <windows.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <cstdio>
#include <codecvt>

std::atomic<bool> stopMonitoring{ false };
std::thread monitorThread;
FILE* logFile = nullptr;

void MonitorDirectory(const std::wstring& directory) {
    if (!logFile) {
        return;
    }

    HANDLE hDirectory = CreateFile(
        directory.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDirectory == INVALID_HANDLE_VALUE) {
        fprintf(logFile, "Failed to open directory handle.\n");
        fclose(logFile);
        return;
    }

    while (!stopMonitoring) {
        fprintf(logFile, "checked directory\n");
        DWORD bytesReturned;
        FILE_NOTIFY_INFORMATION buffer[1024];
        BOOL result = ReadDirectoryChangesW(
            hDirectory, buffer, sizeof(buffer), TRUE,
            FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL);

        if (result) {
            // Logic to handle file change event
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string directoryStr = converter.to_bytes(directory);
            fprintf(logFile, "File changed in directory: %s\n", directoryStr.c_str());
        }
    }

    CloseHandle(hDirectory);
}

extern "C" __declspec(dllexport) void StartMonitoring(std::wstring directory) {
    stopMonitoring = false;
    monitorThread = std::thread(MonitorDirectory, directory);
}

extern "C" __declspec(dllexport) void StopMonitoring() {
    stopMonitoring = true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        logFile = fopen("FileChangeLog.txt", "a");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (monitorThread.joinable())
        {
            monitorThread.join();
        }
        fclose(logFile);
        break;
    }
    return TRUE;
}
