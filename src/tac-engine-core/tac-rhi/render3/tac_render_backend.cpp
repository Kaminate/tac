#include "tac_render_backend.h" // self-inc

#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/mutex/tac_mutex.h"

namespace Tac
{
  struct IdMgr
  {
    int  Alloc();
    void Free( int );

  private:
    IdCollection sCollection;
    Mutex        sMtx;
  };

  int  IdMgr::Alloc()
  {
    TAC_SCOPE_GUARD( LockGuard, sMtx );
    return sCollection.Alloc();
  }

  void IdMgr::Free( int i )
  {
    TAC_SCOPE_GUARD( LockGuard, sMtx );
    return sCollection.Free( i );
  }

  // -----------------------------------------------------------------------------------------------

  static IdMgr sIdMgrs[ ( int )Render::HandleType::kCount ];

  Render::BufferHandle    Render::AllocBufferHandle()    { return sIdMgrs[ ( int )Render::HandleType::kBuffer ].Alloc(); }
  Render::PipelineHandle  Render::AllocPipelineHandle()  { return sIdMgrs[ ( int )Render::HandleType::kPipeline ].Alloc(); }
  Render::ProgramHandle   Render::AllocProgramHandle()   { return sIdMgrs[ ( int )Render::HandleType::kProgram ].Alloc(); }
  Render::SamplerHandle   Render::AllocSamplerHandle()   { return sIdMgrs[ ( int )Render::HandleType::kSampler ].Alloc(); }
  Render::SwapChainHandle Render::AllocSwapChainHandle() { return sIdMgrs[ ( int )Render::HandleType::kSwapChain ].Alloc(); }
  Render::TextureHandle   Render::AllocTextureHandle()   { return sIdMgrs[ ( int )Render::HandleType::kTexture ].Alloc(); }
  void                    Render::FreeHandle( ResourceHandle h ) 
  {
    if( !h.IsValid() )
      return;

    const HandleType type{ h.GetHandleType() };
    const int i{ h.GetIndex() };
    sIdMgrs[ ( int )type ].Free( i );
  }
} // namespace Tac
