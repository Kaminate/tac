#include "tac_render_handle.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-rhi/render/tac_render_context_data.h"
#include "tac-rhi/render/tac_render_backend_cmd_data.h"
#include "tac-rhi/render/tac_render_handle_mgr.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

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
    std::mutex   sFreeIdMutexes[ ( int )HandleType::kCount ];
    Vector< RenderHandle > sFreedRenderHandles[ ( int )HandleType::kCount ];
  } sIDManager;


  // -----------------------------------------------------------------------------------------------

  // RenderHandle

  RenderHandle::RenderHandle( RenderHandle&& rh ) noexcept
  {
    Swap( mHandle, rh.mHandle );
    Swap( mReferenceCounter, rh.mReferenceCounter );
    Swap( mType, rh.mType );
  }

  RenderHandle::RenderHandle( HandleType type, Handle h )
    : mType{ type }
    , mHandle{ h }
  {
  }

  RenderHandle::RenderHandle( const RenderHandle& rh )
  {
    Assign( rh );
  }

  RenderHandle::~RenderHandle()
  {
    Dec();
  }

  void RenderHandle::Clear()
  {
    Dec();
    mReferenceCounter = {};
    mHandle = {};
  }

  void RenderHandle::Inc() const
  {
    if( !mReferenceCounter )
      mReferenceCounter = TAC_NEW int{ mHandle.IsValid() ? 1 : 0 };

    ( *mReferenceCounter )++;
  }

  void RenderHandle::Dec() const
  {
    if( mReferenceCounter )
    {
      int& n = *mReferenceCounter;
      --n;
      if( !n )
      {
        TAC_DELETE mReferenceCounter;
        RenderHandleFree( mType, mHandle );
        mHandle = {};
        mReferenceCounter = {};
      }
    }
    else if( mHandle.IsValid() )
    {
      RenderHandleFree( mType, mHandle );
      mHandle = {};
    }
  }

  int  RenderHandle::GetHandleIndex() const { return mHandle.GetIndex(); }

  HandleType RenderHandle::GetHandleType() const { return mType; }

  bool RenderHandle::IsValid() const         { return mHandle.IsValid(); }

  void RenderHandle::operator = ( RenderHandle&& rh ) noexcept
  {
    Swap( mHandle, rh.mHandle );
    Swap( mReferenceCounter, rh.mReferenceCounter );
    Swap( mType, rh.mType );
  }

  void RenderHandle::operator = ( const RenderHandle& rh )
  {
    Assign( rh );
  }

  void RenderHandle::Assign( const RenderHandle& rh )
  {
    TAC_ASSERT( mType == HandleType::kCount || mType == rh.mType );

    Dec();
    rh.Inc();
    mHandle = rh.mHandle;
    mReferenceCounter = rh.mReferenceCounter;
    mType = rh.mType;
  }


} // namespace Tac::Render

// -------------------------------------------------------------------------------------------------

namespace Tac
{
}


