
/** $VER: Direct3D.h (2024.01.17) P. Stuer **/

#pragma once

#include "framework.h"

class Direct3D
{
public:
    Direct3D();

    HRESULT GetDXGIDevice(IDXGIDevice ** device) const noexcept;

public:
    CComPtr<ID3D11Device> Device;
};

extern Direct3D _Direct3D;
