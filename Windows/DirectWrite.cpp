
/** $VER: DirectWrite.cpp (2024.04.04) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
DirectWrite::DirectWrite()
{
    HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(Factory), (IUnknown **) &Factory);

    if (!SUCCEEDED(hr))
        throw COMException(hr, L"Unable to create DirectWrite factory.");
}

DirectWrite _DirectWrite;
