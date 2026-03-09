#pragma once
#include <d3d11.h>
#include <Windows.h>

struct D3DContext {
    ID3D11Device*            device            = nullptr;
    ID3D11DeviceContext*     deviceContext     = nullptr;
    IDXGISwapChain*          swapChain         = nullptr;
    ID3D11RenderTargetView*  renderTargetView  = nullptr;
};

bool CreateD3DDevice(HWND hwnd, D3DContext& ctx);
void CleanupD3DDevice(D3DContext& ctx);
void CreateRenderTarget(D3DContext& ctx);
void CleanupRenderTarget(D3DContext& ctx);
