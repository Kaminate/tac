#include "tac_window_api.h" // self-inc

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  WindowHandle::WindowHandle( int index ) : mIndex( index ) {}

  using WindowBackend::sGameLogicCurr;

  v2i        WindowHandle::GetPos() const { return sGameLogicCurr[ mIndex ].mPos; }

  v2i        WindowHandle::GetSize() const { return sGameLogicCurr[ mIndex ].mSize; }

  StringView WindowHandle::GetName() const { return sGameLogicCurr[ mIndex ].mName; }

  bool       WindowHandle::IsShown() const { return sGameLogicCurr[ mIndex ].mShown; }

  bool       WindowHandle::IsValid() const { return mIndex != -1; }

  int        WindowHandle::GetIndex() const { return mIndex; }

  // -----------------------------------------------------------------------------------------------

  WindowHandle WindowApi::CreateWindow( CreateParams )
  {
  }

  void         WindowApi::DestroyWindow( WindowHandle )
  {
  }

} // namespace Tac

