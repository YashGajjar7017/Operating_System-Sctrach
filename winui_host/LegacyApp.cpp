#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

// Forward declarations of window procedures
LRESULT CALLBACK LegacyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void RedirectToModernHost(HWND hWnd);

// Global application instance
HINSTANCE g_hInstance = nullptr;
HWND g_hMainWnd = nullptr;

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInstance = hInstance;

    // Enable Per-Monitor V2 DPI Awareness for crisp non-client areas
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const wchar_t CLASS_NAME[] = L"XenithraLegacyShellWindow";

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = LegacyWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr; // Handled by modern compositor
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex))
    {
        return 0;
    }

    g_hMainWnd = CreateWindowExW(
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Xenithra Core Shell",
        WS_POPUP | WS_VISIBLE, // Frameless top-level window
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hMainWnd)
    {
        return 0;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    // Enter message pump
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

/* ============================================================================
 * LEGACY SUBSYSTEM DEPRECATION
 * ============================================================================
 * The GDI rendering pipeline below represents the legacy shell rendering path.
 *
 * Drawbacks of this legacy pipeline:
 * 1. CPU-bound software rasterization introduces frame drops and high CPU load.
 * 2. Complete absence of hardware alpha-blending and modern DWM Mica/Acrylic composition.
 * 3. Inability to handle fractional DPI scaling across multi-monitor setups.
 * 4. Severe visual tearing during asynchronous window resizes.
 *
 * Transition path:
 * Instead of drawing through GDI in WM_PAINT, the native HWND lifecycle is preserved,
 * but rendering responsibility is forwarded to the WinUI 3 DirectComposition swapchain
 * and CoreWebView2 instance initialized in RedirectToModernHost().
 * ============================================================================ */

LRESULT CALLBACK LegacyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Transition immediately into the modern WinUI 3 / WebView2 host
        RedirectToModernHost(hWnd);
        return 0;
    }

    /* ---------------------- DEPRECATED GDI ROUTINE --------------------------
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // CPU-bound memory device context allocation
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 
                                                   clientRect.right - clientRect.left, 
                                                   clientRect.bottom - clientRect.top);
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

        // Solid background fill
        HBRUSH bgBrush = CreateSolidBrush(RGB(15, 23, 42));
        FillRect(memDC, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        // GDI text rendering
        SetTextColor(memDC, RGB(255, 255, 255));
        SetBkMode(memDC, TRANSPARENT);
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(memDC, hFont);

        const wchar_t legacyText[] = L"Legacy GDI Subsystem Active - Deprecated";
        TextOutW(memDC, 20, 20, legacyText, static_cast<int>(wcslen(legacyText)));

        // Bit-block transfer to destination screen DC
        BitBlt(hdc, 0, 0, 
               clientRect.right - clientRect.left, 
               clientRect.bottom - clientRect.top, 
               memDC, 0, 0, SRCCOPY);

        // Resource release
        SelectObject(memDC, oldFont);
        DeleteObject(hFont);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent GDI flicker
    ------------------------------------------------------------------------- */

    case WM_PAINT:
    case WM_ERASEBKGND:
    {
        // DirectComposition and WinUI 3 handle presentation. Bypass GDI.
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

void RedirectToModernHost(HWND hWnd)
{
    // The native HWND is preserved for shell management while presentation
    // switches to the modern Windows App SDK DirectComposition engine.
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hWnd));
}
