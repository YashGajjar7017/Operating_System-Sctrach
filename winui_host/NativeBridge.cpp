#include "pch.h"
#include "NativeBridge.h"
#include "NativeBridge.g.cpp"
#include <sstream>
#include <iomanip>

namespace winrt::ModernShell::implementation
{
    NativeBridge::NativeBridge(HWND rootHwnd, Microsoft::Web::WebView2::Core::CoreWebView2 const& webView)
        : m_rootHwnd(rootHwnd), m_coreWebView(webView)
    {
    }

    uint64_t NativeBridge::FileTimeToUint64(FILETIME const& ft)
    {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }

    void NativeBridge::LaunchProcess(hstring const& executablePath)
    {
        std::wstring targetPath = executablePath.c_str();

        STARTUPINFOW si = { sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION pi = {};

        // Launch process detached from shell process group
        BOOL success = CreateProcessW(
            nullptr,
            targetPath.data(),
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
            BroadcastWebEvent(L"PROCESS_LAUNCHED", L"{\"status\":\"SUCCESS\",\"path\":\"" + targetPath + L"\"}");
        }
        else
        {
            DWORD error = GetLastError();
            std::wostringstream errJson;
            errJson << L"{\"status\":\"FAILED\",\"errorCode\":" << error << L"}";
            BroadcastWebEvent(L"PROCESS_LAUNCH_ERROR", errJson.str());
        }
    }

    void NativeBridge::MinimizeWindow()
    {
        if (m_rootHwnd && IsWindow(m_rootHwnd))
        {
            ShowWindow(m_rootHwnd, SW_MINIMIZE);
        }
    }

    void NativeBridge::CloseShell()
    {
        if (m_rootHwnd && IsWindow(m_rootHwnd))
        {
            PostMessageW(m_rootHwnd, WM_CLOSE, 0, 0);
        }
    }

    hstring NativeBridge::GetSystemTelemetry()
    {
        // 1. Query Physical and Virtual Memory Status
        MEMORYSTATUSEX memStatus = { sizeof(MEMORYSTATUSEX) };
        GlobalMemoryStatusEx(&memStatus);

        // 2. Query System CPU Times
        static FILETIME prevIdleTime = {};
        static FILETIME prevKernelTime = {};
        static FILETIME prevUserTime = {};

        FILETIME idleTime = {};
        FILETIME kernelTime = {};
        FILETIME userTime = {};

        double cpuPercent = 0.0;

        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {
            uint64_t uIdle = FileTimeToUint64(idleTime);
            uint64_t uKernel = FileTimeToUint64(kernelTime);
            uint64_t uUser = FileTimeToUint64(userTime);

            uint64_t uPrevIdle = FileTimeToUint64(prevIdleTime);
            uint64_t uPrevKernel = FileTimeToUint64(prevKernelTime);
            uint64_t uPrevUser = FileTimeToUint64(prevUserTime);

            uint64_t diffIdle = uIdle - uPrevIdle;
            uint64_t diffKernel = uKernel - uPrevKernel;
            uint64_t diffUser = uUser - uPrevUser;
            uint64_t diffTotal = diffKernel + diffUser;

            if (diffTotal > 0 && uPrevKernel > 0)
            {
                cpuPercent = (1.0 - (static_cast<double>(diffIdle) / static_cast<double>(diffTotal))) * 100.0;
                if (cpuPercent < 0.0) cpuPercent = 0.0;
                if (cpuPercent > 100.0) cpuPercent = 100.0;
            }

            prevIdleTime = idleTime;
            prevKernelTime = kernelTime;
            prevUserTime = userTime;
        }

        // 3. Construct JSON buffer
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << L"{"
           << L"\"cpuUsage\":" << cpuPercent << L","
           << L"\"ramPercent\":" << memStatus.dwMemoryLoad << L","
           << L"\"totalPhysicalBytes\":" << memStatus.ullTotalPhys << L","
           << L"\"availPhysicalBytes\":" << memStatus.ullAvailPhys
           << L"}";

        return hstring(ss.str());
    }

    void NativeBridge::BroadcastWebEvent(std::wstring const& eventName, std::wstring const& jsonPayload)
    {
        if (m_coreWebView)
        {
            std::wostringstream messageStream;
            messageStream << L"{"
                          << L"\"event\":\"" << eventName << L"\","
                          << L"\"payload\":" << jsonPayload
                          << L"}";

            // Push message onto the Chromium web message pipeline
            m_coreWebView.PostWebMessageAsJson(messageStream.str());
        }
    }
}
