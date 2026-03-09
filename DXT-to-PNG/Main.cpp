#include "app.h"
#include "converter.h"
#include "gui.h"
#include "gui_state.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <shellapi.h>
#include "resource.h"
#include <filesystem>
#include <print>

// Forward declaration from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static D3DContext g_ctx;
static AppState   g_state;

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return TRUE;

    switch (msg) {
    case WM_SIZE:
        if (g_ctx.device && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget(g_ctx);
            g_ctx.swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam),
                                           DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget(g_ctx);
        }
        return 0;

    case WM_DROPFILES:
        HandleDropFiles(reinterpret_cast<HDROP>(wParam), g_state);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // suppress Alt menu
        break;

    case WM_CLOSE:
        if (g_state.converting.load()) return 0; // block close while converting
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // ── Console mode: run headlessly when arguments are passed ────────────────
    int     argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 2) {
        AttachConsole(ATTACH_PARENT_PROCESS);
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);

        const std::filesystem::path input  = argv[1];
        const std::filesystem::path output = (argc >= 3)
            ? std::filesystem::path(argv[2])
            : input.parent_path() / (input.stem().string() + ".png");
        LocalFree(argv);

        if (const auto result = Convert(input, output); !result) {
            std::println(stderr, "Error: {}", result.error());
            return 1;
        }
        return 0;
    }
    LocalFree(argv);

    // ── GUI mode ──────────────────────────────────────────────────────────────
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"DXTtoPNG";
    wc.hIcon   = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        L"DXTtoPNG", L"DXT to PNG Converter",
        WS_OVERLAPPEDWINDOW, 100, 100, 900, 620,
        nullptr, nullptr, hInstance, nullptr);

    DragAcceptFiles(hwnd, TRUE);

    if (!CreateD3DDevice(hwnd, g_ctx)) {
        CleanupD3DDevice(g_ctx);
        UnregisterClassW(wc.lpszClassName, hInstance);
        CoUninitialize();
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_ctx.device, g_ctx.deviceContext);

    constexpr float kClearColor[4] = { 0.10f, 0.10f, 0.10f, 1.00f };
    bool done = false;

    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderGui(g_state);

        ImGui::Render();
        if (g_ctx.renderTargetView) {
            g_ctx.deviceContext->OMSetRenderTargets(1, &g_ctx.renderTargetView, nullptr);
            g_ctx.deviceContext->ClearRenderTargetView(g_ctx.renderTargetView, kClearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
        g_ctx.swapChain->Present(1, 0); // vsync on
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3DDevice(g_ctx);
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);
    CoUninitialize();
    return 0;
}
