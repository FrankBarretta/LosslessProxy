#include "gui_manager.h"
#include "gui_style.h"
#include "icon_loader.h"
#include "../../third_party/stb_image.h"
#include "tabs/tab_addons.h"
#include "tabs/tab_settings.h"
#include "tabs/tab_logs.h"
#include "tabs/tab_about.h"
#include "widgets/toast.h"
#include "../addon/addon_manager.h"
#include "../host/host_impl.h"
#include "../log/logger.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include <dxgi.h>
#include <filesystem>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Forward declare
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace lsproxy {

// Load a PNG file as a Win32 HICON
static HICON LoadIconFromPng(const std::wstring& path) {
    // Convert wstring to UTF-8 for stb_image
    int sz = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), &utf8[0], sz, nullptr, nullptr);

    int w, h, channels;
    unsigned char* rgba = stbi_load(utf8.c_str(), &w, &h, &channels, 4);
    if (!rgba) return nullptr;

    // Convert RGBA to BGRA (Windows HICON format)
    for (int i = 0; i < w * h; i++) {
        unsigned char tmp = rgba[i * 4 + 0];
        rgba[i * 4 + 0] = rgba[i * 4 + 2];
        rgba[i * 4 + 2] = tmp;
    }

    HBITMAP hColor = CreateBitmap(w, h, 1, 32, rgba);
    stbi_image_free(rgba);
    if (!hColor) return nullptr;

    HBITMAP hMask = CreateCompatibleBitmap(GetDC(NULL), w, h);
    if (!hMask) { DeleteObject(hColor); return nullptr; }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hColor;
    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(hColor);
    DeleteObject(hMask);
    return icon;
}

// D3D11 state
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static AddonManager* g_manager = nullptr;

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

static void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    // Load system DLLs to bypass ReShade
    wchar_t systemPath[MAX_PATH];
    if (!GetSystemDirectoryW(systemPath, MAX_PATH)) return false;
    std::wstring sysDir = systemPath;

    HMODULE hDXGI = LoadLibraryW((sysDir + L"\\dxgi.dll").c_str());
    if (!hDXGI) return false;

    typedef HRESULT(WINAPI* CreateDXGIFactory1Func)(REFIID, void**);
    auto createFactoryFunc = (CreateDXGIFactory1Func)GetProcAddress(hDXGI, "CreateDXGIFactory1");
    if (!createFactoryFunc) return false;

    IDXGIFactory1* pFactory = nullptr;
    if (FAILED(createFactoryFunc(__uuidof(IDXGIFactory1), (void**)&pFactory))) return false;

    HMODULE hD3D11 = LoadLibraryW((sysDir + L"\\d3d11.dll").c_str());
    if (!hD3D11) { pFactory->Release(); return false; }

    typedef HRESULT(WINAPI* D3D11CreateDeviceFunc)(
        IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*,
        UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
    auto createDeviceFunc = (D3D11CreateDeviceFunc)GetProcAddress(hD3D11, "D3D11CreateDevice");
    if (!createDeviceFunc) { pFactory->Release(); return false; }

    if (FAILED(createDeviceFunc(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
                                featureLevelArray, 2, D3D11_SDK_VERSION,
                                &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext))) {
        pFactory->Release();
        return false;
    }

    if (FAILED(pFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain))) {
        g_pd3dDevice->Release();
        g_pd3dDeviceContext->Release();
        pFactory->Release();
        return false;
    }

    pFactory->Release();
    CreateRenderTarget();
    return true;
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void GuiManager::StartGuiThread(AddonManager* manager) {
    g_manager = manager;
    CreateThread(NULL, 0, GuiThread, NULL, 0, NULL);
}

DWORD WINAPI GuiManager::GuiThread(LPVOID lpParam) {
    // Load addons in GUI thread to avoid loader lock
    if (g_manager) {
        g_manager->LoadAddons();
    }

    // Load window icon from icon.png next to the DLL
    HICON hAppIcon = nullptr;
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring iconPath = std::filesystem::path(exePath).parent_path() / L"addons" / L".." / L"LP-icon.png";
        // Normalize: try next to the exe first, then next to the proxy DLL
        std::wstring iconPath1 = (std::filesystem::path(exePath).parent_path() / L"LP-icon.png").wstring();
        HMODULE hSelf = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&GuiManager::GuiThread, &hSelf);
        wchar_t dllPath[MAX_PATH];
        GetModuleFileNameW(hSelf, dllPath, MAX_PATH);
        std::wstring iconPath2 = (std::filesystem::path(dllPath).parent_path() / L"LP-icon.png").wstring();

        if (std::filesystem::exists(iconPath1))
            hAppIcon = LoadIconFromPng(iconPath1);
        else if (std::filesystem::exists(iconPath2))
            hAppIcon = LoadIconFromPng(iconPath2);
    }

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
                       GetModuleHandle(NULL), hAppIcon, NULL, NULL, NULL,
                       L"LosslessProxyV2Class", hAppIcon };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Lossless Scaling Addons Manager v0.3.0",
                              WS_OVERLAPPEDWINDOW, 100, 100, 1050, 700,
                              NULL, NULL, wc.hInstance, NULL);

    // Set icon on window (in case wc didn't apply it)
    if (hAppIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hAppIcon);
    }

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Set up icon loader with D3D11 device and load addon icons
    IconLoader_SetDevice(g_pd3dDevice);
    if (g_manager) {
        g_manager->LoadAddonIcons();
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Initialize addons with ImGui context
    if (g_manager) {
        g_manager->InitializeAddons(ImGui::GetCurrentContext());
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupModernStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clearColor = ImVec4(0.08f, 0.07f, 0.07f, 1.0f);
    bool done = false;

    LOG_INFO("GUI", "Addon Manager window ready");

    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Free GetConfig return buffers from the previous frame
        if (g_manager && g_manager->GetHost())
            g_manager->GetHost()->FlushReturnBuffers();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Fullscreen main window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Main", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Tab bar
        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Addons")) {
                ImGui::Dummy(ImVec2(0, 5));
                RenderTabAddons(g_manager);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings")) {
                RenderTabSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Logs")) {
                ImGui::Dummy(ImVec2(0, 5));
                RenderTabLogs();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About")) {
                RenderTabAbout();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();

        // Toast overlay
        widgets::ToastRender();

        // Render
        ImGui::Render();
        const float cc[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

} // namespace lsproxy
