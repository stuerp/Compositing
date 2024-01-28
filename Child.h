
/** $VER: Child.h (2024.01.28) P. Stuer **/

#pragma once

#include "framework.h"

class Child
{
public:
    Child();
    ~Child();

    HRESULT Initialize(HWND hParent);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT Render();

    LRESULT OnResize(UINT width, UINT height);

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceDependentResources();
    void DeleteBitmapSourceDependentResources();
    void DeleteDeviceDependentResources();

    void ResizeSwapChain(UINT width, UINT height) noexcept;
    HRESULT CreateSwapChainBuffers(ID2D1DeviceContext * dc, IDXGISwapChain1 * swapChain) noexcept;

    HRESULT CreateBitmapSource(IWICBitmapSource ** bitmapSource) const noexcept;
    HRESULT CreateBitmap(IWICBitmapSource * bitmapSource, ID2D1RenderTarget * renderTarget, UINT maxWidth, UINT maxHeight, ID2D1Bitmap ** bitmap) const noexcept;

private:
    HWND _hWnd;

    uint32_t _Number;

    CComPtr<IDXGIDevice> _DXGIDevice;
    CComPtr<ID2D1Device1> _D2DDevice;
    CComPtr<IDCompositionDevice> _CompositionDevice;

    CComPtr<ID2D1DeviceContext> _DC;
    CComPtr<IDXGISwapChain1> _SwapChain;

    CComPtr<IDCompositionTarget> _CompositionTarget;
    CComPtr<IDCompositionVisual> _CompositionVisual;

    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    CComPtr<IWICBitmapSource> _BitmapSource;
    CComPtr<ID2D1Bitmap> _Bitmap;

    const WCHAR * ClassName = L"Compositing.Child";
};
