#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac::Render
{
  static int sMaxGPUFrameCount; 
  static Filesystem::Path sShaderOutputPath; 

  static IRenderBackend3* sBackend;

  // -----------------------------------------------------------------------------------------------
  Handle::Handle( int i ) : mIndex( i ) {}
  int Handle::GetIndex() const { TAC_ASSERT( IsValid() ); return mIndex; }
  bool Handle::IsValid() const { return mIndex != -1; }
  // -----------------------------------------------------------------------------------------------

  template< typename T >
  struct IdProtT
  {
    static T Alloc()
    {
      TAC_SCOPE_GUARD( std::lock_guard, GetMtx() );
      return GetIds().Alloc();
    }

    static void Free(T t )
    {
      TAC_SCOPE_GUARD( std::lock_guard, GetMtx() );
      return GetIds().Free( ( ( Handle )t ).GetIndex() );
    }

  private:

    static IdCollection& GetIds()
    {
      static IdCollection sCollection;
      return sCollection;
    }

    static std::mutex& GetMtx()
    {
      static std::mutex sMtx;
      return sMtx;
    }
  };

  // -----------------------------------------------------------------------------------------------

  void RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sShaderOutputPath = params.mShaderOutputPath;
    sBackend = IRenderBackend3::sInstance;
    sBackend->Init( errors );
  }

  Filesystem::Path RenderApi::GetShaderOutputPath()
  {
    return sShaderOutputPath;
  }

  int RenderApi::GetMaxGPUFrameCount()
  {
    return sMaxGPUFrameCount;
  }

  FBHandle RenderApi::CreateFB( FrameBufferParams params, Errors& errors )
  {
    const FBHandle h = IdProtT<FBHandle>::Alloc();
    sBackend->CreateFB( h, params, errors );
    return h; 
  }

  void     RenderApi::ResizeFB( FBHandle h, v2i size )
  {
    sBackend->ResizeFB( h, size );
  }

  TexFmt         RenderApi::GetFBFmt( FBHandle h )
  {
    return sBackend->GetFBFmt( h );
  }

  void     RenderApi::DestroyFB( FBHandle h )
  {
    sBackend->DestroyFB( h );
    IdProtT<FBHandle>::Free( h );
  }

  DynBufHandle RenderApi::CreateDynBuf( int n, StackFrame sf, Errors& errors )
  {
    const DynBufHandle h = IdProtT< DynBufHandle >::Alloc();
    sBackend->CreateDynBuf( h, n, sf, errors );
    return h;
  }

  void         RenderApi::UpdateDynBuf( UpdateDynBufParams params )
  {
    sBackend->UpdateDynBuf( params );
  }

  void         RenderApi::DestroyDynBuf( DynBufHandle h )
  {
    sBackend->DestroyDynBuf( h );
    IdProtT<DynBufHandle>::Free( h );
  }

  ProgramHandle RenderApi::CreateShaderProgram( ShaderProgramParams params, Errors& errors )
  {
    const ProgramHandle h = IdProtT< ProgramHandle >::Alloc();
    sBackend->CreateShaderProgram( h, params, errors );
    return h;
  }

  void           RenderApi::DestroyShaderProgram( ProgramHandle h )
  {
    sBackend->DestroyShaderProgram( h );
    IdProtT<ProgramHandle>::Free( h );
  }

  PipelineHandle RenderApi::CreateRenderPipeline( PipelineParams params, Errors& errors )
  {
    const PipelineHandle h = IdProtT< PipelineHandle >::Alloc();
    sBackend->CreateRenderPipeline( h, params, errors );
    return h;
  }

  void           RenderApi::DestroyRenderPipeline( PipelineHandle h )
  {
    sBackend->DestroyRenderPipeline( h );
    IdProtT<PipelineHandle>::Free( h );
  }
}
