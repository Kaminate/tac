#include "tac_jppt.h" // self-inc


#include "tac-rhi/render3/tac_render_api.h"
//#include "tac-engine-core/window/tac_window_handle.h"
//#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/os/tac_os.h"
//#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
//#include "tac-ecs/ghost/tac_ghost.h"
//#include "tac-ecs/tac_space.h"

#include "tac_jppt_BVH.h"
#include "tac_jppt_cornell_box.h"

namespace Tac
{
  static v2i                      sWindowPos;
  static v2i                      sWindowSize;
  static const char*              sWindowName{ "JPPT" };
  static Render::TextureHandle    sTexture;
  static Render::ProgramHandle    sProgram;
  static Render::PipelineHandle   sPipeline;

  static gpupt::Scene*            sScene;
  static gpupt::SceneBVH*         sSceneBVH;

  JPPTApp::JPPTApp( Config cfg ) : App( cfg )
  {
  }


  void    JPPTApp::CreateTexture( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    // kRGBA8_unorm_srgb cannot be used with typed uav
    const Render::TexFmt fmt{ Render::TexFmt::kRGBA8_unorm };

    const Render::Image image
    {
      .mWidth   { sWindowSize.x },
      .mHeight  { sWindowSize.y },
      .mDepth   { 1 },
      .mFormat  { fmt },
    };

    const Render::Binding binding
    {
      Render::Binding::ShaderResource | // ImGuiImage input 
      Render::Binding::UnorderedAccess // compute shader output 
    };

    const Render::CreateTextureParams createTextureParams
    {
      .mImage                  { image },
      .mMipCount               { 1 },
      .mBinding                { binding },
      .mUsage                  { Render::Usage::Default },
      .mCpuAccess              { Render::CPUAccess::None },
      .mOptionalName           { "JPPT" }
    };
    TAC_CALL( sTexture = renderDevice->CreateTexture( createTextureParams, errors ) );
  }

  void    JPPTApp::Init( InitParams initParams, Errors& errors)
  {
    const Monitor monitor{ OS::OSGetPrimaryMonitor() };
    sWindowSize = v2i( ( int )( monitor.mSize.x * 0.8f ),
                       ( int )( monitor.mSize.y * 0.8f ) );
    sWindowPos = ( monitor.mSize - sWindowSize ) / 2;

    TAC_CALL( CreateTexture( errors ) );
    //TAC_CALL( CreatePipeline( errors ) );

    TAC_CALL( sScene = gpupt::Scene::CreateCornellBox( errors ) );
    TAC_CALL( sSceneBVH = gpupt::SceneBVH::CreateBVH( sScene, errors ) );
  }

  void    JPPTApp::CreatePipeline( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs{ "jppt" },
    };

    TAC_CALL( sProgram = renderDevice->CreateProgram( programParams, errors ) );
    const Render::PipelineParams pipelineParams
    {
      .mProgram{ sProgram },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    Render::IShaderVar* outputTexture {
      renderDevice->GetShaderVariable( sPipeline, "sOutputTexture" ) };
    outputTexture->SetResource( sTexture );
  }

  void    JPPTApp::Update( UpdateParams updateParams, Errors& )
  {
    ImGuiSetNextWindowDisableBG();
    ImGuiSetNextWindowPosition( sWindowPos );
    ImGuiSetNextWindowSize( sWindowSize );
    if( ImGuiBegin( sWindowName ) )
    {
      CornellBox::DebugImGui();
      ImGuiSetCursorPos( {} );
      ImGuiImage( sTexture.GetIndex(), sWindowSize );
      ImGuiEnd();
    }
  }

  void    JPPTApp::Render( RenderParams renderParams, Errors& errors )
  {
    if( true )
      return;

    const WindowHandle windowHandle{ ImGuiGetWindowHandle(sWindowName)};
    const SysWindowApi windowApi{ renderParams.mWindowApi};
    const bool shown{ windowApi.IsShown( windowHandle ) };
    if( !shown )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::SwapChainHandle swapChain{ windowApi.GetSwapChainHandle( windowHandle ) };
    const Render::TextureHandle swapChainColor{
      renderDevice->GetSwapChainCurrentColor( swapChain ) };
    //const Render::TextureHandle swapChainDepth{
    //  renderDevice->GetSwapChainDepth( swapChain ) };
    TAC_CALL( Render::IContext::Scope renderContextScope{
      renderDevice->CreateRenderContext( errors ) } );

    Render::IContext* renderContext{ renderContextScope.GetContext() };
    //const Render::Targets renderTargets
    //{
    //  .mColors { swapChainColor },
    //  .mDepth  { swapChainDepth },
    //};

    const float t{ ( float )Sin( renderParams.mTimestamp.mSeconds * 2.0 ) * 0.5f + 0.5f };

    const v3i threadGroupCounts( RoundUpToNearestMultiple( sWindowSize.x, 8 ),
                                 RoundUpToNearestMultiple( sWindowSize.y, 8 ),
                                 1 );

    //renderContext->SetRenderTargets( renderTargets );
    renderContext->ClearColor( swapChainColor, v4( t, 0, 1, 1 ) );

    renderContext->SetPipeline( sPipeline );
    renderContext->CommitShaderVariables();
    renderContext->Dispatch( threadGroupCounts );
    renderContext->SetSynchronous(); // imgui happens after

    TAC_CALL( renderContext->Execute( errors ) );
  }

  void    JPPTApp::Present( PresentParams, Errors& )
  {
  }

  void    JPPTApp::Uninit( Errors& )
  {
  }

  App* App::Create()
  {
    const App::Config config
    {
       .mName { "JPPT" },
    };
    return TAC_NEW JPPTApp( config );
  }


}// namespace Tac

