// Variants - put a single number in C:\Temp\wv2_variant.txt (no rebuild or repackage needed):
//   0 = no WebView2 (baseline)
//   1 = create WebView2 only, no navigation
//   2 = create WebView2 + navigate (default if the file is missing)
//   3 = same as 2, but call ICoreWebView2Controller::Close() before exit

#pragma once

#include <WebView2.h>
#include <wrl.h>
#include <cwchar>
#include <fstream>

namespace Repro
{
    static const wchar_t* kStartUrl = L"https://www.bing.com";
    static const wchar_t* kUserDataFolder = nullptr; // nullptr = WebView2 default location

    static int g_variant = 3;
    static Microsoft::WRL::ComPtr<ICoreWebView2Environment> g_env;
    static Microsoft::WRL::ComPtr<ICoreWebView2Controller> g_controller;
    static Microsoft::WRL::ComPtr<ICoreWebView2> g_webview;

    static void ShowError(const wchar_t* step, HRESULT hr)
    {
        wchar_t msg[128];
        swprintf_s(msg, L"WebView2 %s failed: 0x%08X", step, static_cast<unsigned>(hr));
        MessageBoxW(nullptr, msg, L"WebView2 Repro", MB_OK | MB_ICONERROR);
    }

    static void LoadVariant()
    {
        std::ifstream file("C:\\Temp\\wv2_variant.txt");
        int value = 0;
        if (file >> value && value >= 0 && value <= 3)
        {
            g_variant = value;
        }
    }

    // Call once, right after the game window has been created.
    static void CreateWebView(HWND hwnd)
    {
        using namespace Microsoft::WRL;

        LoadVariant();

 wchar_t title[64];
  swprintf_s(title, L"WebView2 Repro - variant %d", g_variant);
  SetWindowTextW(hwnd, title);

        if (g_variant == 0)
        {
            return;
        }

        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr, kUserDataFolder, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
                {
                    if (FAILED(result) || !env)
                    {
                        ShowError(L"environment creation", result);
                        return result;
                    }
                    g_env = env;

                    return env->CreateCoreWebView2Controller(hwnd,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                            {
                                if (FAILED(result) || !controller)
                                {
                                    ShowError(L"controller creation", result);
                                    return result;
                                }
                                g_controller = controller;
                                g_controller->get_CoreWebView2(&g_webview);

                                // Show WebView2 in the left half of the window
                                RECT bounds = {};
                                GetClientRect(hwnd, &bounds);
                                bounds.right = bounds.left + (bounds.right - bounds.left) / 2;
                                g_controller->put_Bounds(bounds);

                                if (g_variant >= 2 && g_webview)
                                {
                                    g_webview->Navigate(kStartUrl);
                                }
                                return S_OK;
                            }).Get());
                }).Get());

        if (FAILED(hr))
        {
            ShowError(L"CreateCoreWebView2EnvironmentWithOptions", hr);
        }
    }

    // Call once, after the message loop ends and before the game shuts down.
    static void CloseWebView()
    {
        if (g_variant == 3 && g_controller)
        {
            g_controller->Close();
        }
        g_webview.Reset();
        g_controller.Reset();
        g_env.Reset();
    }
}