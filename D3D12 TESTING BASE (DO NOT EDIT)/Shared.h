#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>

#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d12.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler
#pragma comment(lib, "dxguid.lib")

// D3D12 extension library.
#include "d3dx12.h"

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

//Miscellaneous
#include <stdio.h>
#include <exception>
#include <chrono>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

constexpr WCHAR wndClassName[] = L"Window Class";

inline UINT GetStringByteSize(const char* string) {
    UINT ret = 0;
    while (*(string++))
        ret++;
    return ret;
}

using namespace DirectX;

inline XMFLOAT2 normalize(XMFLOAT2 p) {
    float m = sqrt(p.x * p.x + p.y * p.y);
    if (m) {
        p.x /= m;
        p.y /= m;
    }
    return p;
}

inline XMFLOAT3 normalize(XMFLOAT3 p) {
    float m = p.x * p.x + p.y * p.y + p.z * p.z;
    if (m) {
        p.x /= m;
        p.y /= m;
        p.z /= m;
    }
    return p;
}

inline XMFLOAT4 normalize(XMFLOAT4 p) {
    float m = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (m) {
        p.x /= m;
        p.y /= m;
        p.z /= m;
    }
    return p;
}

inline XMFLOAT3 cross(XMFLOAT3 p, XMFLOAT3 q) {
    return { p.y * q.z - p.z * q.y, p.z * q.x - p.x * q.z, p.x * q.y - p.y * q.x };
}

inline XMFLOAT3 subtract(XMFLOAT3 p, XMFLOAT3 q) {
    return { p.x - q.x,p.y - q.y,p.z - q.z };
}

inline float dot(XMFLOAT3 x, XMFLOAT3 y) {
    return x.x * y.x + x.y * y.y + x.z * y.z;
}

inline float dot(XMFLOAT2 x, XMFLOAT2 y) {
    return x.x * y.x + x.y * y.y;
}