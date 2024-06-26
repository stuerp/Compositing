
/** $VER: Direct3D.cpp (2024.04.04) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Direct3D.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
Direct3D::Direct3D()
{
#ifdef _DEBUG
    const DWORD Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG; // Enables interoperability with Direct2D.
 #else
    const DWORD Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; // Enables interoperability with Direct2D.
#endif

    HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, Flags, nullptr, 0, D3D11_SDK_VERSION, &Device, nullptr, nullptr);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create Direct3D device.");
}

/// <summary>
/// Gets the DXGI interface.
/// </summary>
HRESULT Direct3D::GetDXGIDevice(IDXGIDevice ** device) const noexcept
{
    return Device.QueryInterface(device);
}

Direct3D _Direct3D;
