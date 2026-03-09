#include "app.h"
#include <dxgi.h>

bool CreateD3DDevice(HWND hwnd, D3DContext& ctx) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = 0;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel{};
    const HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &ctx.swapChain, &ctx.device, &featureLevel, &ctx.deviceContext);
    if (FAILED(hr)) return false;

    CreateRenderTarget(ctx);
    return true;
}

void CleanupD3DDevice(D3DContext& ctx) {
    CleanupRenderTarget(ctx);
    if (ctx.swapChain)     { ctx.swapChain->Release();     ctx.swapChain     = nullptr; }
    if (ctx.deviceContext) { ctx.deviceContext->Release(); ctx.deviceContext = nullptr; }
    if (ctx.device)        { ctx.device->Release();        ctx.device        = nullptr; }
}

void CreateRenderTarget(D3DContext& ctx) {
    ID3D11Texture2D* backBuffer = nullptr;
    ctx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer) {
        ctx.device->CreateRenderTargetView(backBuffer, nullptr, &ctx.renderTargetView);
        backBuffer->Release();
    }
}

void CleanupRenderTarget(D3DContext& ctx) {
    if (ctx.renderTargetView) {
        ctx.renderTargetView->Release();
        ctx.renderTargetView = nullptr;
    }
}
