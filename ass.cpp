#include <windows.h>
#include <gdiplus.h>
#include <wininet.h>
#include <iostream>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")

using namespace Gdiplus;

// This entry point ensures no console window pops up
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Initialize GDI+ for potential screenshot logic
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Main stealth loop
    while (true) {
        // [Screenshot and Webhook Logic would go here]
        // Currently just sleeps to keep the process alive in the background
        
        Sleep(60000); // Wait 1 minute
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
