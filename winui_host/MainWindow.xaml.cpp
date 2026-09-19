#include "pch.h"
#include "MainWindow.xaml.h"
#include "MainWindow.g.cpp"
#include "NativeBridge.h"

#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.Foundation.h>
#include <shlobj.h>
#include <filesystem>

#pragma comment(lib, "Dwmapi.lib")

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Windowing;
using namespace Microsoft::Web::WebView2::Core;

namespace winrt::ModernShell::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        // Retrieve native Win32 HWND from WinUI 3 Window
        auto windowNative = this->try_as<::IWindowNative>();
        check_hresult(windowNative->get_WindowHandle(&m_hwnd));

        // Configure system dark mode and Mica backdrop via DWM
        SetupDwmAttributes();

        // Make window frameless and extend content into title area
        auto appWindow = this->AppWindow();
        auto titleBar = appWindow.TitleBar();
        titleBar.ExtendsContentIntoTitleBar(true);
        titleBar.ButtonBackgroundColor(Windows::UI::Colors::Transparent());
        titleBar.ButtonInactiveBackgroundColor(Windows::UI::Colors::Transparent());

        // Construct root container grid
        Grid rootGrid;
        rootGrid.Background(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));

        m_webView = WebView2();
        m_webView.HorizontalAlignment(HorizontalAlignment::Stretch);
        m_webView.VerticalAlignment(VerticalAlignment::Stretch);

        // Prevent initial white screen flicker before Chromium renders
        m_webView.DefaultBackgroundColor(Windows::UI::Colors::Transparent());

        rootGrid.Children().Append(m_webView);
        this->Content(rootGrid);

        // Initialize embedded web runtime
        InitializeWebView2();
    }

    void MainWindow::SetupDwmAttributes()
    {
        if (!m_hwnd) return;

        // 1. Force DWM Immersive Dark Mode
        BOOL useDarkMode = TRUE;
        check_hresult(DwmSetWindowAttribute(
            m_hwnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE,
            &useDarkMode,
            sizeof(useDarkMode)
        ));

        // 2. Enable Windows 11 Mica System Backdrop (DWMSBT_MAINWINDOW = 2)
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_MAINWINDOW;
        check_hresult(DwmSetWindowAttribute(
            m_hwnd,
            DWMWA_SYSTEMBACKDROP_TYPE,
            &backdropType,
            sizeof(backdropType)
        ));

        // 3. Extend window frame into client area for seamless glass rendering
        MARGINS margins = { -1, -1, -1, -1 };
        check_hresult(DwmExtendFrameIntoClientArea(m_hwnd, &margins));
    }

    fire_and_forget MainWindow::InitializeWebView2()
    {
        auto strongThis{ get_strong() };

        // Define separate User Data Folder for shell storage isolation
        PWSTR localAppData = nullptr;
        check_hresult(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData));
        std::wstring userDataFolder = std::wstring(localAppData) + L"\\XenithraShell\\WebView2Cache";
        CoTaskMemFree(localAppData);

        // Chromium engine options: eliminate network/telemetry background tasks
        auto envOptions = make<CoreWebView2EnvironmentOptions>();
        envOptions.AdditionalBrowserArguments(
            L"--disable-background-networking "
            L"--disable-features=TranslateUI "
            L"--disable-sync "
            L"--enable-features=msWebView2EnableDraggableRegions "
            L"--in-process-gpu"
        );

        auto env = co_await CoreWebView2Environment::CreateWithOptionsAsync(
            L"",
            userDataFolder.c_str(),
            envOptions
        );

        co_await m_webView.EnsureCoreWebView2Async(env);

        auto coreWebView = m_webView.CoreWebView2();
        auto settings = coreWebView.Settings();

        // Lockdown embedded browser settings
        settings.IsScriptEnabled(true);
        settings.IsWebMessageEnabled(true);
        settings.AreDefaultContextMenusEnabled(false);
        settings.AreDevToolsEnabled(true);
        settings.IsStatusBarEnabled(false);
        settings.AreDefaultScriptDialogsEnabled(false);

        // Map local disk folder to secure virtual origin https://app.local/
        std::wstring assetsDirectory = std::filesystem::current_path().wstring() + L"\\www";
        coreWebView.SetVirtualHostNameToFolderMapping(
            L"app.local",
            assetsDirectory.c_str(),
            COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
        );

        // Instantiate and inject the C++/WinRT broker object into V8 runtime
        auto nativeBridge = make<NativeBridge>(m_hwnd, coreWebView);
        coreWebView.AddHostObjectToScript(L"nativeHost", nativeBridge);

        // Navigate to the local virtual web host
        coreWebView.Navigate(L"https://app.local/index.html");
    }
}
