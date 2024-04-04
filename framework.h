
/** $VER: framework.h (2024.04.04) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#define NOMINMAX
#include <windows.h>

#include <atlbase.h>

#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1helper.h>
#include <dcomp.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dcomp")
#pragma comment(lib, "windowscodecs")

#include <stdlib.h>
#include <strsafe.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>

#include <cmath>

#include "COMException.h"

#ifndef Assert
#if defined(DEBUG) || defined(_DEBUG)
#define Assert(b) do {if (!(b)) { ::OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif
#endif

#define TOSTRING_IMPL(x) #x
#define TOSTRING(x) TOSTRING_IMPL(x)

#ifndef THIS_HINSTANCE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE ((HINSTANCE) &__ImageBase)
#endif

inline int ToDPI(int pixels, UINT dpi)
{
    return (int) ::ceil((float) pixels * (float) dpi / USER_DEFAULT_SCREEN_DPI);
}
