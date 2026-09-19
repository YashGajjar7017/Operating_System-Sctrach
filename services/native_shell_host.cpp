/**
 * @file native_shell_host.cpp
 * @brief Headless C++ Shell Backend & Process Manager — Xenithra OS Custom Shell
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

// ============================================================================
// LEGACY GDI RENDERING PIPELINE (DEPRECATED & STRIPPED)
// ============================================================================
/*
LRESULT CALLBACK DeprecatedWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // DEPRECATED: Software double buffering via GDI
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
        HGDIOBJ oldBmp = SelectObject(memDC, hBmp);

        HBRUSH bgBrush = CreateSolidBrush(RGB(15, 23, 42));
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        SetTextColor(memDC, RGB(255, 255, 255));
        SetBkMode(memDC, TRANSPARENT);
        TextOutW(memDC, 10, 10, L"Legacy GDI Rendering - Deprecated", 33);

        BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(hBmp);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
*/

// ============================================================================
// CONSTANTS & GLOBALS
// ============================================================================
static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\CustomShellIPC";
static const wchar_t* ELECTRON_PATH = L"C:\\CustomShell\\electron-app.exe";
static const wchar_t* ELECTRON_ARGS = L"C:\\CustomShell\\electron-app.exe --kiosk --no-sandbox";
static const wchar_t* WINLOGON_SHELL_KEY = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

static std::atomic<bool> g_running{ true };
static HANDLE g_hElectronProcess = nullptr;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool EnableShutdownPrivilege()
{
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }

    TOKEN_PRIVILEGES tp = {};
    LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
    CloseHandle(hToken);
    return (result && GetLastError() == ERROR_SUCCESS);
}

bool SpawnProcess(const std::wstring& targetPath)
{
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmdLine = targetPath;

    BOOL success = CreateProcessW(
        nullptr,
        &cmdLine[0],
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (success)
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
}

bool RestoreExplorerShell()
{
    HKEY hKey = nullptr;
    LONG status = RegOpenKeyExW(HKEY_CURRENT_USER, WINLOGON_SHELL_KEY, 0, KEY_SET_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
    {
        status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, WINLOGON_SHELL_KEY, 0, KEY_SET_VALUE, &hKey);
    }

    if (status == ERROR_SUCCESS)
    {
        const wchar_t* explorerStr = L"explorer.exe";
        RegSetValueExW(
            hKey,
            L"Shell",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(explorerStr),
            static_cast<DWORD>((wcslen(explorerStr) + 1) * sizeof(wchar_t))
        );
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// ============================================================================
// ELECTRON PROCESS MONITOR & SUPERVISOR
// ============================================================================

void LaunchAndMonitorElectron()
{
    while (g_running.load())
    {
        STARTUPINFOW si = { sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION pi = {};
        std::wstring cmdLine = ELECTRON_ARGS;

        BOOL created = CreateProcessW(
            nullptr,
            &cmdLine[0],
            nullptr,
            nullptr,
            FALSE,
            CREATE_UNICODE_ENVIRONMENT,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (created)
        {
            CloseHandle(pi.hThread);
            g_hElectronProcess = pi.hProcess;

            // Block worker thread until Electron process terminates
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            g_hElectronProcess = nullptr;
        }

        // If shell backend is still supposed to run, pause before restarting crashed Electron instance
        if (g_running.load())
        {
            Sleep(1500);
        }
    }
}

// ============================================================================
// COMMAND PARSER & IPC ROUTER
// ============================================================================

void ProcessJsonCommand(const std::string& requestStr, std::string& responseStr)
{
    auto ExtractValue = [&](const std::string& key) -> std::string {
        size_t keyPos = requestStr.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return "";
        size_t colonPos = requestStr.find(':', keyPos);
        if (colonPos == std::string::npos) return "";
        size_t startQuote = requestStr.find('"', colonPos);
        if (startQuote == std::string::npos) return "";
        size_t endQuote = requestStr.find('"', startQuote + 1);
        if (endQuote == std::string::npos) return "";
        return requestStr.substr(startQuote + 1, endQuote - startQuote - 1);
    };

    std::string action = ExtractValue("action");

    if (action == "launch")
    {
        std::string targetA = ExtractValue("target");
        std::wstring targetW(targetA.begin(), targetA.end());
        bool launched = SpawnProcess(targetW);
        responseStr = launched ? "{\"status\":\"ok\",\"launched\":true}" : "{\"status\":\"error\",\"code\":1}";
    }
    else if (action == "power")
    {
        std::string type = ExtractValue("type");
        EnableShutdownPrivilege();
        if (type == "shutdown")
        {
            ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
            responseStr = "{\"status\":\"ok\",\"power\":\"shutdown\"}";
        }
        else if (type == "reboot")
        {
            ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
            responseStr = "{\"status\":\"ok\",\"power\":\"reboot\"}";
        }
        else
        {
            responseStr = "{\"status\":\"error\",\"message\":\"unknown_power_type\"}";
        }
    }
    else if (action == "restore_explorer")
    {
        RestoreExplorerShell();
        responseStr = "{\"status\":\"ok\",\"explorer_restored\":true}";
        
        g_running.store(false);
        if (g_hElectronProcess)
        {
            TerminateProcess(g_hElectronProcess, 0);
        }
        PostQuitMessage(0);
    }
    else
    {
        responseStr = "{\"status\":\"error\",\"message\":\"invalid_action\"}";
    }
}

// ============================================================================
// NAMED PIPE SERVER LOOP
// ============================================================================

void RunPipeServer()
{
    while (g_running.load())
    {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            4096,
            4096,
            0,
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            Sleep(1000);
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected)
        {
            char buffer[2048] = {};
            DWORD bytesRead = 0;

            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                std::string requestStr(buffer);
                std::string responseStr;

                ProcessJsonCommand(requestStr, responseStr);

                DWORD bytesWritten = 0;
                WriteFile(hPipe, responseStr.c_str(), static_cast<DWORD>(responseStr.size()), &bytesWritten, nullptr);
                FlushFileBuffers(hPipe);
            }
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}

// ============================================================================
// MAIN ENTRYPOINT (HEADLESS BACKGROUND SHELL MANAGER)
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // 1. Launch supervisor thread for Electron application lifecycle
    std::thread monitorThread(LaunchAndMonitorElectron);

    // 2. Launch IPC Pipe server on main thread loop
    RunPipeServer();

    // 3. Cleanup on exit
    if (monitorThread.joinable())
    {
        monitorThread.join();
    }

    return 0;
}
