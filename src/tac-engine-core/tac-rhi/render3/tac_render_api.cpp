#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Render
{
  struct IdProt
  {
    void FreeIdx( int );
    int AllocIdx();
    IdCollection mIDs;
    std::mutex   mMtx;
  };

  IdProt sIDPropFB;
  IdProt sIDPropDB;

  static int sMaxGPUFrameCount; 

  static IRenderBackend3* sBackend;

  Handle::Handle( int i ) : mIndex( i ) {}
  int Handle::GetIndex() const { TAC_ASSERT( IsValid() ); return mIndex; }
  bool Handle::IsValid() const { return mIndex != -1; }

  void IdProt::FreeIdx( int i)
  {
    TAC_SCOPE_GUARD( std::lock_guard, mMtx );
    mIDs.Free( i );
  }

  int IdProt::AllocIdx()
  {
    TAC_SCOPE_GUARD( std::lock_guard, mMtx );
    return mIDs.Alloc();
  }

  static FBHandle     AllocIdxFrameBuf()            { return sIDPropFB.AllocIdx(); }
  static void         FreeIdxFrameBuf( FBHandle h ) { sIDPropFB.FreeIdx( h.GetIndex() ); }

  static DynBufHandle AllocIdxDynBuf()                { return sIDPropDB.AllocIdx(); }
  static void         FreeIdxDynBuf( DynBufHandle h ) { sIDPropDB.FreeIdx( h.GetIndex() ); }


  // -----------------------------------------------------------------------------------------------

  void RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sBackend = IRenderBackend3::sInstance;
    sBackend->Init( errors );
  }

  int RenderApi::GetMaxGPUFrameCount()
  {
    return sMaxGPUFrameCount;
  }

  FBHandle RenderApi::CreateFB( const void* nwh, v2i size, Errors& errors )
  {
    const FBHandle h = AllocIdxFrameBuf();
    sBackend->CreateFB( h, nwh, size, errors );
    return h; 
  }

  void     RenderApi::ResizeFB( FBHandle h, v2i size )
  {
    sBackend->ResizeFB( h, size );
  }


  void     RenderApi::DestroyFB( FBHandle h )
  {
    sBackend->DestroyFB( h, FreeIdxFrameBuf );
  }

  DynBufHandle RenderApi::CreateDynBuf( int n, StackFrame sf, Errors& errors )
  {
    const DynBufHandle h = AllocIdxDynBuf();
    sBackend->CreateDynBuf( h, n, sf, errors );
    return h;
  }

  void         RenderApi::UpdateDynBuf( UpdateDynBufParams params )
  {
    sBackend->UpdateDynBuf( params );
  }

  void         RenderApi::DestroyDynBuf( DynBufHandle h )
  {
    sBackend->DestroyDynBuf( h, FreeIdxDynBuf );
  }
}
