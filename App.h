
/** $VER: App.h (2024.01.17) P. Stuer **/

#pragma once

#include "framework.h"

class App
{
public:
    App();
    ~App();

    HRESULT Initialize();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HRESULT Render();

    LRESULT OnResize(UINT width, UINT height);
    LRESULT OnDropFiles(HDROP hDrop);
    LRESULT OnKeyDown(WPARAM wParam);

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceDependentResources();
    void DeleteBitmapSourceDependentResources();
    void DeleteDeviceDependentResources();

    void ResizeSwapChain(UINT width, UINT height) noexcept;
    HRESULT CreateSwapChainBuffers(ID2D1DeviceContext * dc, IDXGISwapChain1 * swapChain) noexcept;

    HRESULT CreateGridPatternBrush(ID2D1RenderTarget * renderTarget, ID2D1BitmapBrush ** bitmapBrush) const noexcept;
    HRESULT CreateBitmapSource(IWICBitmapSource ** bitmapSource) const noexcept;
    HRESULT CreateBitmap(IWICBitmapSource * bitmapSource, ID2D1RenderTarget * renderTarget, const D2D1_SIZE_U & size, ID2D1Bitmap ** bitmap) const noexcept;

private:
    HWND _hWnd;

    uint32_t _Number;
    WCHAR _FilePath[MAX_PATH];
    WCHAR _Message[256];

    CComPtr<IDXGIDevice> _DXGIDevice;
    CComPtr<ID2D1Device1> _D2DDevice;
    CComPtr<IDCompositionDevice> _CompositionDevice;

    CComPtr<IDWriteTextFormat> _TextFormat;

    CComPtr<ID2D1DeviceContext> _DC;
    CComPtr<IDXGISwapChain1> _SwapChain;

    CComPtr<IDCompositionTarget> _CompositionTarget;
    CComPtr<IDCompositionVisual> _CompositionVisual;

    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    CComPtr<ID2D1BitmapBrush> _BackgroundBrush;
    CComPtr<IWICBitmapSource> _BitmapSource;
    CComPtr<ID2D1Bitmap> _Bitmap;

    const WCHAR * ClassName = L"Compositing";
    const WCHAR * WindowTitle = L"Compositing";
};
