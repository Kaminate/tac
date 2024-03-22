#include "tac_render_handle_mgr.h" // self-inc

#include "tac-rhi/identifier/tac_id_collection.h"

import std; // mutex

namespace Tac::Render
{
  static struct
  {
    Handle AllocHandleOfType( HandleType type )
    {
      TAC_SCOPE_GUARD( std::lock_guard, sIdMutexes[ ( int )type ] );
      const int i = sIdCollections[ ( int )type ].Alloc();
      return Handle{ i };
    }

    void FreeHandleOfType( HandleType type, Handle h )
    {
      TAC_SCOPE_GUARD( std::lock_guard, sIdMutexes[ ( int )type ] );
      sIdCollections[ ( int )type ].Free( h.GetIndex() );
    }

  private:
    IdCollection sIdCollections[ ( int )HandleType::kCount ];
    std::mutex   sIdMutexes[ ( int )HandleType::kCount ];
  } sIDManager;
} // namespace Tac::Render


namespace Tac
{
  Handle Render::RenderHandleAlloc( HandleType type )
  {
    return sIDManager.AllocHandleOfType( type );
  }

  void   Render::RenderHandleFree( HandleType type , Handle h)
  {
    sIDManager.FreeHandleOfType( type, h );
  }
}
