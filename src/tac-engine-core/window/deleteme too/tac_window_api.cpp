#include "tac_window_api.h" // self-inc

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-engine-core/window/tac_window_backend.h"

import std; // mutex

namespace Tac
{
  using WindowBackend::sGameLogicCurr;


  WindowHandle::WindowHandle( int index ) : mIndex( index ) {}

  v2i        WindowHandle::GetPosi() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mPos;
  }

  v2         WindowHandle::GetPosf() const
  {
    const v2i v = GetPosi();
    return { ( float )v.x, ( float )v.y };
  }

  int        WindowHandle::GetX() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mPos.x;
  }

  int        WindowHandle::GetY() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mPos.y;
  }

  v2i        WindowHandle::GetSizei() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mSize;
  }

  v2         WindowHandle::GetSizef() const
  {
    const v2i v = GetSizei();
    return { ( float )v.x, ( float )v.y };
  }

  int        WindowHandle::GetWidth() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mSize.x;
  }

  int        WindowHandle::GetHeight() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mSize.y;
  }

  StringView WindowHandle::GetName() const
  {
    TAC_ASSERT( IsValid() );
    return sGameLogicCurr[ mIndex ].mName;
  }

  bool       WindowHandle::IsShown() const { return IsValid() && sGameLogicCurr[ mIndex ].mShown; }

  bool       WindowHandle::IsValid() const { return mIndex != -1; }

  int        WindowHandle::GetIndex() const { return mIndex; }

  // -----------------------------------------------------------------------------------------------

  WindowHandle WindowApi::CreateWindow( CreateParams params )
  {
    return WindowBackend::GameLogicCreateWindow( params );
  }

  void         WindowApi::DestroyWindow( WindowHandle h )
  {
    WindowBackend::GameLogicDestroyWindow( h );
  }

} // namespace Tac

