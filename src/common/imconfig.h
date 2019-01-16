//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
// This file contains compile-time options for ImGui.
// Other options (memory allocation overrides, callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------

#pragma once

#include "common/math/tacVector2.h"
#include "common/math/tacVector4.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"

//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Include imgui_user.h at the end of imgui.h
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Don't implement default handlers for Windows (so as not to link with OpenClipboard() and others Win32 functions)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS

//---- Don't implement test window functionality (ShowTestWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
//---- It is very strongly recommended to NOT disable the test windows. Please read the comment at the top of imgui_demo.cpp to learn why.
//#define IMGUI_DISABLE_TEST_WINDOWS

//---- Don't define obsolete functions names
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Pack colors to BGRA instead of RGBA (remove need to post process vertex buffer in back ends)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Implement STB libraries in a namespace to avoid conflicts
#define IMGUI_STB_NAMESPACE     ImGuiStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const v2& f) { x = f.x; y = f.y; }                           \
        operator v2() const { return v2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const v4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }         \
        operator v4() const { return v4(x,y,z,w); }

//---- Use 32-bit vertex indices (instead of default: 16-bit) to allow meshes with more than 64K vertices
//#define ImDrawIdx unsigned int

namespace ImGui
{
  bool InputText( const char* label, TacString& str );
  void Text( const TacString& s );
  bool Button( const TacString& s );
  bool Checkbox( const TacString& s, bool* b );
  bool DragDouble( const TacString& s, double* d );
}

