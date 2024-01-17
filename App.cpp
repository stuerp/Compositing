
/** $VER: App.cpp (2024.01.17) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "App.h"

#include "Direct3D.h"
#include "Direct2D.h"
#include "DirectWrite.h"

#include <chrono>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
App::App() : _hWnd(), _Number(1), _FilePath(), _Message()
{
}

/// <summary>
/// Destructs this instance.
/// </summary>
App::~App()
{
    DeleteDeviceDependentResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
HRESULT App::Initialize()
{
    HRESULT hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
    {
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

        wcex.lpszClassName = ClassName;
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = App::WndProc;
        wcex.hInstance = THIS_HINSTANCE;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = NULL;
        wcex.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);

        ::RegisterClassExW(&wcex);

        const DWORD Style = WS_OVERLAPPEDWINDOW;
        const DWORD ExStyle = WS_EX_NOREDIRECTIONBITMAP; // Disable the creation of the opaque redirection surface.

        _hWnd = ::CreateWindowExW(ExStyle, ClassName, WindowTitle, Style, 0, 0, 0, 0, NULL, NULL, THIS_HINSTANCE, this);

        hr = _hWnd ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        UINT DPI = ::GetDpiForWindow(_hWnd);

        ::MoveWindow(_hWnd, 0, 0, (int) ::ceil(640.f * (float) DPI / 96.f), (int) ::ceil(480.f * (float) DPI / 96.f), TRUE);

        ::ShowWindow(_hWnd, SW_SHOWNORMAL);
        ::UpdateWindow(_hWnd);
    }

    return hr;
}

/// <summary>
/// Windows procedure
/// </summary>
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
        App * This = (App *) pcs->lpCreateParams;

        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(This));

        ::DragAcceptFiles(hWnd, TRUE);

        return 1;
    }
 
    App * This = (App *)(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

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

            case WM_DROPFILES:
                return This->OnDropFiles((HDROP) wParam);

            case WM_KEYDOWN:
                return This->OnKeyDown(wParam);

            case WM_DESTROY:
            {
                ::PostQuitMessage(0);

                return 1;
            }
        }
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

/// <summary>
/// Handles the WM_SIZE message.
/// </summary>
LRESULT App::OnResize(UINT width, UINT height)
{
    if (_DC == nullptr)
        return 0;

    _Bitmap.Release(); // Ensure that the bitmap gets rescaled.

    ResizeSwapChain(width, height);

    return 0;
}

/// <summary>
/// Handles the WM_DROPFILES message.
/// </summary>
LRESULT App::OnDropFiles(HDROP hDrop)
{
    if (::DragQueryFileW(hDrop, 0, _FilePath, _countof(_FilePath)) != 0)
    {
        DeleteBitmapSourceDependentResources();

        ::InvalidateRect(_hWnd, nullptr, TRUE);
    }

    ::DragFinish(hDrop);

    return 0;
}

/// <summary>
/// Handles the WM_KEYDOWN message.
/// </summary>
LRESULT App::OnKeyDown(WPARAM wParam)
{
    switch (wParam)
    {
        default:
            return 1;

        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
    }

    return 0;
}

/// <summary>
/// Renders a frame.
/// </summary>
HRESULT App::Render()
{
    HRESULT hr = CreateDeviceDependentResources();

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F RenderTargetSize = _DC->GetSize();

        _DC->BeginDraw();

        _DC->SetTransform(D2D1::Matrix3x2F::Identity());

        // Draw the circle.
        {
            const D2D1_POINT_2F Center = D2D1::Point2F(RenderTargetSize.width / 2.f, RenderTargetSize.height / 2.f);
            const FLOAT Radius = ((std::min)(RenderTargetSize.width, RenderTargetSize.height) / 2.f) - 8.f;
            const D2D1_ELLIPSE Ellipse = D2D1::Ellipse(Center, Radius, Radius);

            _SolidBrush->SetColor(D2D1::ColorF(0.18f, 0.55f, 0.34f, 0.75f));
            _DC->FillEllipse(Ellipse, _SolidBrush);
        }

        if (_Bitmap)
        {
            D2D1_SIZE_F Size = _Bitmap->GetSize();

            D2D1_RECT_F Rect = D2D1::RectF((RenderTargetSize.width - Size.width) / 2.f, (RenderTargetSize.height - Size.height) / 2.f, Size.width, Size.height);

            Rect.right += Rect.left;
            Rect.bottom += Rect.top;

            _DC->DrawBitmap(_Bitmap, Rect);
        }

        if (_Message[0])
        {
            _SolidBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
            _DC->DrawText(_Message, (UINT) ::wcslen(_Message), _TextFormat, D2D1::RectF(0.f, 0.f, RenderTargetSize.width, RenderTargetSize.height), _SolidBrush);
        }

        _DC->EndDraw();

        // Present the swap chain to the composition engine.
        hr = _SwapChain->Present(1, 0);

        if (!SUCCEEDED(hr) && (hr != DXGI_STATUS_OCCLUDED))
            DeleteDeviceDependentResources();
    }

    return hr;
}

// Create resources which are not bound to any device. Their lifetime effectively extends for the duration of the app. These resources include the Direct2D,
// DirectWrite, and WIC factories; and a DirectWrite Text Format object (used for identifying particular font characteristics) and a Direct2D geometry.
HRESULT App::CreateDeviceIndependentResources()
{
    HRESULT hr = _Direct3D.GetDXGIDevice(&_DXGIDevice);

    // Create the Direct2D device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = _Direct2D.Factory->CreateDevice(_DXGIDevice, &_D2DDevice);

    // Create the DirectComposition device that links back to the Direct3D device.
    if (SUCCEEDED(hr))
        hr = ::DCompositionCreateDevice(_DXGIDevice, __uuidof(_CompositionDevice), (void **) &_CompositionDevice);

    if (SUCCEEDED(hr))
    {
        static const WCHAR FontName[] = L"Verdana";
        static const FLOAT FontSize = 24.f;

        hr = _DirectWrite.Factory->CreateTextFormat(FontName, NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);
    }

    if (SUCCEEDED(hr))
    {
        _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    return hr;
}

//  Creates resources which are bound to a particular Direct3D device. It's all centralized here, in case the resources
//  need to be recreated in case of Direct3D device loss (eg. display change, remoting, removal of video card, etc).
HRESULT App::CreateDeviceDependentResources()
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

    if (SUCCEEDED(hr) && (_BackgroundBrush == nullptr))
        hr = CreateGridPatternBrush(_DC, &_BackgroundBrush);

    if (SUCCEEDED(hr) && (_SolidBrush == nullptr))
        hr = _DC->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &_SolidBrush);
/*
    if (SUCCEEDED(hr) && (_BitmapSource == nullptr))
        hr = CreateBitmapSource(&_BitmapSource);

    if (SUCCEEDED(hr) && (_Bitmap == nullptr))
        hr = CreateBitmap(_BitmapSource, _RenderTarget, RenderTargetSize, &_Bitmap);
*/
    return hr;
}

//
// Creates a pattern brush.
//
HRESULT App::CreateGridPatternBrush(ID2D1RenderTarget * renderTarget, ID2D1BitmapBrush ** bitmapBrush) const noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(10.0f, 10.0f), &rt);

    CComPtr<ID2D1SolidColorBrush> GridBrush;

    if (SUCCEEDED(hr))
        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)), &GridBrush);

    if (SUCCEEDED(hr))
    {
        rt->BeginDraw();

        rt->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), GridBrush);
        rt->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), GridBrush);

        hr = rt->EndDraw();
    }

    CComPtr<ID2D1Bitmap> GridBitmap;

    if (SUCCEEDED(hr))
        hr = rt->GetBitmap(&GridBitmap);

    if (SUCCEEDED(hr))
    {
        D2D1_BITMAP_BRUSH_PROPERTIES BrushProperties = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

        hr = rt->CreateBitmapBrush(GridBitmap, BrushProperties, bitmapBrush);
    }

    return hr;
}

/// <summary>
/// Creates the bitmap source.
/// </summary>
HRESULT App::CreateBitmapSource(IWICBitmapSource ** bitmapSource) const noexcept
{
    if (_FilePath[0] != 0)
        return _Direct2D.Load(_FilePath, bitmapSource);

    WCHAR ResourceName[64] = { };

    ::swprintf_s(ResourceName, _countof(ResourceName), L"Image%02d", _Number);

    return _Direct2D.Load(ResourceName, L"Image", bitmapSource);
}

/// <summary>
/// Creates a Direct2D bitmap from the bitmap source.
/// </summary>
HRESULT App::CreateBitmap(IWICBitmapSource * bitmapSource, ID2D1RenderTarget * renderTarget, const D2D1_SIZE_U & size, ID2D1Bitmap ** bitmap) const noexcept
{
    UINT Width = 0, Height = 0;

    HRESULT hr = bitmapSource->GetSize(&Width, &Height);

    CComPtr<IWICBitmapScaler> Scaler;

    // Fit big images.
    if (SUCCEEDED(hr) && ((Width > size.width) || (Height > size.height)))
        hr = _Direct2D.CreateScaler(_BitmapSource, Width, Height, size.width, size.height, &Scaler);

    if (SUCCEEDED(hr))
        hr = _Direct2D.CreateBitmap(Scaler ? Scaler : bitmapSource, renderTarget, bitmap);

    return hr;
}

/// <summary>
/// Discards device-specific resources related to a bitmap source.
/// </summary>
void App::DeleteBitmapSourceDependentResources()
{
    _Bitmap.Release();
    _BitmapSource.Release();
}

/// <summary>
/// Discards device-specific resources which need to be recreated when a Direct3D device is lost.
/// </summary>
void App::DeleteDeviceDependentResources()
{
    DeleteBitmapSourceDependentResources();

    _SolidBrush.Release();
    _BackgroundBrush.Release();

    _SwapChain.Release();
    _DC.Release();
}

/// <summary>
/// Resizes the swap chain buffers.
/// </summary>
void App::ResizeSwapChain(UINT width, UINT height) noexcept
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
HRESULT App::CreateSwapChainBuffers(ID2D1DeviceContext * dc, IDXGISwapChain1 * swapChain) noexcept
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

/// <summary>
/// Entry point
/// </summary>
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    ::HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

    if (SUCCEEDED(::CoInitialize(nullptr)))
    {
        {
            App app;

            if (SUCCEEDED(app.Initialize()))
            {
                MSG Msg;

                while (::GetMessageW(&Msg, NULL, 0, 0))
                {
                    ::TranslateMessage(&Msg);
                    ::DispatchMessageW(&Msg);
                }
            }
        }

        ::CoUninitialize();
    }

    return 0;
}
