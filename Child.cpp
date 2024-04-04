
/** $VER: Child.cpp (2024.04.04) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Child.h"

#include "Direct3D.h"
#include "Direct2D.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Child::Child() : _hWnd(), _Number(2)
{
}

/// <summary>
/// Destructs this instance.
/// </summary>
Child::~Child()
{
    DeleteDeviceDependentResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT Child::Initialize(HWND hParent)
{
    const DWORD Style = WS_CHILDWINDOW;
    const DWORD ExStyle = WS_EX_NOREDIRECTIONBITMAP; // Disable the creation of the opaque redirection surface.

    HRESULT hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
    {
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

        wcex.lpszClassName = ClassName;
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = Child::WndProc;
        wcex.hInstance = THIS_HINSTANCE;
        wcex.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
        wcex.cbWndExtra = sizeof(LONG_PTR);

        ::RegisterClassExW(&wcex);

        _hWnd = ::CreateWindowExW(ExStyle, ClassName, nullptr, Style, 0, 0, 0, 0, hParent, NULL, THIS_HINSTANCE, this);

        hr = _hWnd ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        RECT wr = { 0, 0, 144, 144 };

        ::AdjustWindowRectEx(&wr, Style, FALSE, ExStyle);

        UINT DPI = ::GetDpiForWindow(_hWnd);

        ::MoveWindow(_hWnd, ToDPI(16, DPI), ToDPI(16, DPI), ToDPI(wr.right, DPI), ToDPI(wr.bottom, DPI), TRUE);

        ::ShowWindow(_hWnd, SW_SHOWNORMAL);
        ::UpdateWindow(_hWnd);
    }

    return hr;
}

/// <summary>
/// Windows procedure
/// </summary>
LRESULT CALLBACK Child::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
        Child * This = (Child *) pcs->lpCreateParams;

        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(This));

        ::DragAcceptFiles(hWnd, TRUE);

        return 1;
    }
 
    Child * This = (Child *)(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (This != nullptr)
    {
        switch (msg)
        {
            case WM_SIZE:
                return This->OnResize(LOWORD(lParam), HIWORD(lParam));

            case WM_PAINT:
            case WM_DISPLAYCHANGE:
            {
                PAINTSTRUCT ps;

                ::BeginPaint(hWnd, &ps);

                This->Render();

                ::EndPaint(hWnd, &ps);

                return 0;
            }
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
LRESULT Child::OnResize(UINT width, UINT height)
{
    if (_DC == nullptr)
        return 0;

    _Bitmap.Release(); // Ensure that the bitmap gets rescaled.

    ResizeSwapChain(width, height);

    return 0;
}

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT Child::Render()
{
    HRESULT hr = CreateDeviceDependentResources();

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F RenderTargetSize = _DC->GetSize();

        _DC->BeginDraw();

        _DC->Clear(D2D1::ColorF(0, 0.f));

        if (_Bitmap)
        {
            D2D1_SIZE_F Size = _Bitmap->GetSize();

            D2D1_RECT_F Rect = D2D1::RectF((RenderTargetSize.width - Size.width) / 2.f, (RenderTargetSize.height - Size.height) / 2.f, Size.width, Size.height);

            Rect.right += Rect.left;
            Rect.bottom += Rect.top;

            _DC->DrawBitmap(_Bitmap, Rect);
        }

        _DC->EndDraw();

        // Present the swap chain to the composition engine.
        hr = _SwapChain->Present(1, 0);

        if (!SUCCEEDED(hr) && (hr != DXGI_STATUS_OCCLUDED))
            DeleteDeviceDependentResources();
    }

    return hr;
}

/// <summary>
/// Create resources which are not bound to any device. Their lifetime effectively extends for the duration of the app. These resources include the Direct2D,
/// DirectWrite, and WIC factories; and a DirectWrite Text Format object (used for identifying particular font characteristics) and a Direct2D geometry.
/// </summary>
HRESULT Child::CreateDeviceIndependentResources()
{
    HRESULT hr = _Direct3D.GetDXGIDevice(&_DXGIDevice);

    // Create the Direct2D device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = _Direct2D.Factory->CreateDevice(_DXGIDevice, &_D2DDevice);

    // Create the DirectComposition device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = ::DCompositionCreateDevice(_DXGIDevice, __uuidof(_CompositionDevice), (void **) &_CompositionDevice);

    return hr;
}

/// <summary>
/// Creates resources which are bound to a particular Direct3D device. It's all centralized here, in case the resources
/// need to be recreated in case of Direct3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Child::CreateDeviceDependentResources()
{
    RECT cr = { };

    ::GetClientRect(_hWnd, &cr);

    UINT Width  = (UINT) (cr.right  - cr.left);
    UINT Height = (UINT) (cr.bottom - cr.top);

    HRESULT hr = (Width != 0) && (Height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    // Create the Direct2D device context that is the actual render target and exposes drawing commands.
    if (SUCCEEDED(hr) && (_DC == nullptr))
    {
        hr = _D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_DC);

        // Set the DPI of the device context based on that of the target window.
        if (SUCCEEDED(hr))
        {
            FLOAT DPI = (FLOAT) ::GetDpiForWindow(_hWnd);

            _DC->SetDpi(DPI, DPI);
            _DC->SetTransform(D2D1::Matrix3x2F::Identity());
        }
    }

    // Create the swap chain.
    if (SUCCEEDED(hr) && (_SwapChain == nullptr))
    {
        hr = _DXGI.CreateSwapChain(_DXGIDevice, Width, Height, &_SwapChain);

        if (SUCCEEDED(hr))
            hr = CreateSwapChainBuffers(_DC, _SwapChain);
    }

    // Create the composition target.
    if (SUCCEEDED(hr) && (_CompositionTarget == nullptr))
    {
        hr = _CompositionDevice->CreateTargetForHwnd(_hWnd, true, &_CompositionTarget);

        if (SUCCEEDED(hr) && (_CompositionVisual == nullptr))
            hr = _CompositionDevice->CreateVisual(&_CompositionVisual);

        if (SUCCEEDED(hr))
            hr = _CompositionTarget->SetRoot(_CompositionVisual);
    }

    if (SUCCEEDED(hr))
        hr = _CompositionVisual->SetContent(_SwapChain);

    if (SUCCEEDED(hr))
        hr = _CompositionDevice->Commit();

    if (SUCCEEDED(hr) && (_BitmapSource == nullptr))
        hr = CreateBitmapSource(&_BitmapSource);

    if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        hr = CreateBitmap(_BitmapSource, _DC, Width, Height, &_Bitmap);

    return hr;
}

/// <summary>
/// Creates the bitmap source.
/// </summary>
HRESULT Child::CreateBitmapSource(IWICBitmapSource ** bitmapSource) const noexcept
{
    WCHAR ResourceName[64] = { };

    ::swprintf_s(ResourceName, _countof(ResourceName), L"Image%02d", _Number);

    return _Direct2D.Load(ResourceName, L"Image", bitmapSource);
}

/// <summary>
/// Creates a Direct2D bitmap from the bitmap source.
/// </summary>
HRESULT Child::CreateBitmap(IWICBitmapSource * bitmapSource, ID2D1RenderTarget * renderTarget, UINT maxWidth, UINT maxHeight, ID2D1Bitmap ** bitmap) const noexcept
{
    UINT Width = 0, Height = 0;

    HRESULT hr = bitmapSource->GetSize(&Width, &Height);

    CComPtr<IWICBitmapScaler> Scaler;

    // Fit big images.
    if (SUCCEEDED(hr) && ((Width > maxWidth) || (Height > maxHeight)))
        hr = _Direct2D.CreateScaler(_BitmapSource, Width, Height, maxWidth, maxHeight, &Scaler);

    if (SUCCEEDED(hr))
        hr = _Direct2D.CreateBitmap(Scaler ? Scaler : bitmapSource, renderTarget, bitmap);

    return hr;
}

/// <summary>
/// Discards device-specific resources related to a bitmap source.
/// </summary>
void Child::DeleteBitmapSourceDependentResources()
{
    _Bitmap.Release();
    _BitmapSource.Release();
}

/// <summary>
/// Discards device-specific resources which need to be recreated when a Direct3D device is lost.
/// </summary>
void Child::DeleteDeviceDependentResources()
{
    DeleteBitmapSourceDependentResources();

    _SwapChain.Release();
    _DC.Release();
}

/// <summary>
/// Resizes the swap chain buffers.
/// </summary>
void Child::ResizeSwapChain(UINT width, UINT height) noexcept
{
    // Release the reference to the swap chain before resizing its buffers.
    _DC->SetTarget(nullptr);

    HRESULT hr = (width != 0) && (height != 0) ? S_OK : DXGI_ERROR_INVALID_CALL;

    if (SUCCEEDED(hr))
        hr = _SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    if (SUCCEEDED(hr))
        CreateSwapChainBuffers(_DC, _SwapChain);
    else
        DeleteDeviceDependentResources();
}

/// <summary>
/// Creates the swap chain buffers.
/// </summary>
HRESULT Child::CreateSwapChainBuffers(ID2D1DeviceContext * dc, IDXGISwapChain1 * swapChain) noexcept
{
    // Retrieve the swap chain's back buffer.
    CComPtr<IDXGISurface2> Surface;

    HRESULT hr = swapChain->GetBuffer(0, __uuidof(Surface), (void **) &Surface);

    // Create a Direct2D bitmap that points to the swap chain surface.
    CComPtr<ID2D1Bitmap1> SurfaceBitmap;

    if (SUCCEEDED(hr) && (SurfaceBitmap == nullptr))
    {
        D2D1_BITMAP_PROPERTIES1 Properties = {};

        Properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        Properties.pixelFormat.format    = DXGI_FORMAT_B8G8R8A8_UNORM;
        Properties.bitmapOptions         = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = _DC->CreateBitmapFromDxgiSurface(Surface, Properties, &SurfaceBitmap);

        // Set the surface bitmap as the target of the device context for rendering.
        if (SUCCEEDED(hr))
            _DC->SetTarget(SurfaceBitmap);
    }

    return hr;
}
