#include "tac_render_backend.h" // self-inc

#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/mutex/tac_mutex.h"

namespace Tac
{
  struct IdMgr
  {
    auto Alloc() -> int;
    void Free( int );

  private:
    IdCollection sCollection;
    Mutex        sMtx;
  };

  auto IdMgr::Alloc() -> int
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

  auto Render::AllocBufferHandle() -> BufferHandle       { return sIdMgrs[ ( int )Render::HandleType::kBuffer ].Alloc(); }
  auto Render::AllocPipelineHandle() -> PipelineHandle   { return sIdMgrs[ ( int )Render::HandleType::kPipeline ].Alloc(); }
  auto Render::AllocProgramHandle() -> ProgramHandle     { return sIdMgrs[ ( int )Render::HandleType::kProgram ].Alloc(); }
  auto Render::AllocSamplerHandle() -> SamplerHandle     { return sIdMgrs[ ( int )Render::HandleType::kSampler ].Alloc(); }
  auto Render::AllocSwapChainHandle() -> SwapChainHandle { return sIdMgrs[ ( int )Render::HandleType::kSwapChain ].Alloc(); }
  auto Render::AllocTextureHandle() -> TextureHandle     { return sIdMgrs[ ( int )Render::HandleType::kTexture ].Alloc(); }
  void Render::FreeHandle( ResourceHandle h ) 
  {
    if( !h.IsValid() )
      return;

    const HandleType type{ h.GetHandleType() };
    const int i{ h.GetIndex() };
    sIdMgrs[ ( int )type ].Free( i );
  }
} // namespace Tac
