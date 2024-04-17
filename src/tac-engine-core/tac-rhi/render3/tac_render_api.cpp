#include "tac_render_api.h" //self-inc
#include "tac_render_backend.h"
#include "tac-rhi/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
}

namespace Tac::Render
{
  static int              sMaxGPUFrameCount; 
  static Filesystem::Path sShaderOutputPath; 
  static IDevice*         sDevice;

  // -----------------------------------------------------------------------------------------------
  Handle::Handle( int i ) : mIndex( i ) {}
  int Handle::GetIndex() const { TAC_ASSERT( IsValid() ); return mIndex; }
  bool Handle::IsValid() const { return mIndex != -1; }
  // -----------------------------------------------------------------------------------------------

  void             RenderApi::Init( InitParams params, Errors& errors )
  {
    sMaxGPUFrameCount = params.mMaxGPUFrameCount;
    sShaderOutputPath = params.mShaderOutputPath;
  }

  int              RenderApi::GetMaxGPUFrameCount()
  {
    return sMaxGPUFrameCount;
  }

  Filesystem::Path RenderApi::GetShaderOutputPath()
  {
    return sShaderOutputPath;
  }

  Context          RenderApi::CreateRenderContext( Errors& errors )
  {
    return Context{ .mContextBackend = sBackend->CreateRenderContextBackend( errors ) };
  }

  IDevice* RenderApi::GetRenderDevice()
  {
    return sDevice;
  }

  // -----------------------------------------------------------------------------------------------

  Context::~Context()                                  { mContextBackend->Retire(); }
  void Context::SetViewport( v2i size )                { mContextBackend->SetViewport( size ); }
  void Context::SetScissor( v2i size )                 { mContextBackend->SetScissor( size ); }
  void Context::SetRenderTarget( FBHandle h )          { mContextBackend->SetRenderTarget( h ); }
  void Context::Execute( Errors& errors )              { mContextBackend->Execute( errors ); }
  void Context::ExecuteSynchronously( Errors& errors ) { mContextBackend->ExecuteSynchronously( errors ); }
  void Context::DebugEvent( StringView s )             { mContextBackend->DebugEvent( s ); }
  void Context::DebugMarker( StringView s )            { mContextBackend->DebugMarker( s ); }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
