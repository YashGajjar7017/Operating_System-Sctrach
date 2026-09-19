#pragma once
#include "MainWindow.g.h"
#include <windows.h>
#include <dwmapi.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>

namespace winrt::ModernShell::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

    private:
        HWND m_hwnd{ nullptr };
        Microsoft::UI::Xaml::Controls::WebView2 m_webView{ nullptr };

        void SetupDwmAttributes();
        fire_and_forget InitializeWebView2();
    };
}

namespace winrt::ModernShell::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
