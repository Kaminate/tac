// Precompiled header

#pragma once

#if _MSC_VER

#include <immintrin.h> // Include intel simd intrinsics on x64 and x86 architectures

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <Windowsx.h>
#undef CreateWindow

#include <d3d12.h>
#include <dxgi1_5.h>


#endif // #if _MSC_VER

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_stack_frame.h"
#include "tac-std-lib/string/tac_string.h"



