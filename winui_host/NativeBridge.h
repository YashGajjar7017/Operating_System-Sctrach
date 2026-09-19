#pragma once
#include "NativeBridge.g.h"
#include <windows.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <string>

namespace winrt::ModernShell::implementation
{
    struct NativeBridge : NativeBridgeT<NativeBridge>
    {
        NativeBridge() = default;
        NativeBridge(HWND rootHwnd, Microsoft::Web::WebView2::Core::CoreWebView2 const& webView);

        void LaunchProcess(hstring const& executablePath);
        void MinimizeWindow();
        void CloseShell();
        hstring GetSystemTelemetry();

        // Native -> JavaScript asynchronous broadcast mechanism
        void BroadcastWebEvent(std::wstring const& eventName, std::wstring const& jsonPayload);

    private:
        HWND m_rootHwnd{ nullptr };
        Microsoft::Web::WebView2::Core::CoreWebView2 m_coreWebView{ nullptr };
        
        static uint64_t FileTimeToUint64(FILETIME const& ft);
    };
}

namespace winrt::ModernShell::factory_implementation
{
    struct NativeBridge : NativeBridgeT<NativeBridge, implementation::NativeBridge>
    {
    };
}
