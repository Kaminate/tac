#include "tac_level_editor.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"

#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_sys_window_api.h"

#include "tac-engine-core/shell/tac_shell_timestep.h"

//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render_handles.h"

namespace Tac
{
  Creation     sCreation;
  WindowHandle sWindowHandle;
  const v2i    sWindowSize( 800, 600 );

  // -----------------------------------------------------------------------------------------------

  struct LevelEditorState : public App::IState
  {
  };

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& );
    void Init( InitParams, Errors& ) override;
    void Update( UpdateParams, Errors& ) override;
    void Uninit( Errors& ) override;
    void Render( RenderParams, Errors& ) override;
    void Present( PresentParams, Errors& ) override;
    IState* GetGameState() override;
  };

  LevelEditorApp::LevelEditorApp( const Config& cfg ) : App( cfg ) {}

  void LevelEditorApp::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi* windowApi { initParams.mWindowApi };

    const WindowCreateParams windowCreateParams
    {
      .mName { "level editor" },
      .mPos  { 50, 50 },
      .mSize { sWindowSize }, 
    };

    sWindowHandle = windowApi->CreateWindow( windowCreateParams, errors );
    sCreation.Init( errors );

    {
      //Render::ViewHandle2 viewHandle = Render::CreateView2();
    }
    ++asdf;

  }

  void LevelEditorApp::Update( UpdateParams, Errors& errors )
  {
    sCreation.Update( errors );
  }

  void LevelEditorApp::Uninit( Errors& errors )
  {
    sCreation.Uninit( errors );
  }

  void LevelEditorApp::Present( PresentParams presentParams, Errors& errors )
  {
     const SysWindowApi* windowApi{ presentParams.mWindowApi };
    if( !windowApi->IsShown( sWindowHandle ) )
      return;

    const Render::SwapChainHandle swapChainHandle{
      windowApi->GetSwapChainHandle( sWindowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    TAC_CALL( renderDevice->Present( swapChainHandle, errors ) );
  }

  void LevelEditorApp::Render( RenderParams renderParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ renderParams.mWindowApi };
    if( !windowApi->IsShown( sWindowHandle ) )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::IContext::Scope renderContext{ renderDevice->CreateRenderContext( errors ) };
    const Render::SwapChainHandle swapChainHandle{ windowApi->GetSwapChainHandle( sWindowHandle ) };
    Render::TextureHandle colorTexture{ renderDevice->GetSwapChainCurrentColor( swapChainHandle ) };
    const v4 clearColor{ 0, 0, 0, 1 };
    renderContext->ClearColor( colorTexture, clearColor );
    renderContext->Execute( errors );

  }

  App::IState* LevelEditorApp::GetGameState() 
  {
    return TAC_NEW LevelEditorState;
  }

  // -----------------------------------------------------------------------------------------------

  App* App::Create()
  {
    const Config config
    {
      .mName            { "Level Editor" },
    };

    return TAC_NEW LevelEditorApp( config );
  }

  // -----------------------------------------------------------------------------------------------

  void                Creation::Init( Errors& errors )
  {
  }
  

  void                Creation::Uninit( Errors& errors )
  {
  }
  

  void                Creation::Update( Errors& errors )
  {
    mShowUnownedWindow = false;
    mShowOwnedWindow = true;

    if( mShowOwnedWindow )
    {
      ImGuiSetNextWindowHandle( sWindowHandle );
      if( ImGuiBegin( "Unowned Window" ) )
      {
        UI2DDrawData* drawData{ ImGuiGetDrawData() };

        const UI2DDrawData::Box redBox
        {
          .mMini  { 50, 50 },
          .mMaxi  { 100, 150 },
          .mColor { 1, 0, 0, 1 },
        };

        const UI2DDrawData::Box blueBox
        {
          .mMini  { 200, 200 },
          .mMaxi  { 380, 300 },
          .mColor { 0, 0, 1, 1 },
        };

        const UI2DDrawData::Box greenBox
        {
          .mMini  { 280, 130 },
          .mMaxi  { 340 , 500},
          .mColor { 0, 1, 0, 0.5f },
        };


        const UI2DDrawData::Text text
        {
          .mPos      { 61, 68 },
          .mFontSize { 23 },
          .mUtf8     { "a" },
        };

        //drawData->AddBox( redBox );
        //drawData->AddBox( blueBox );
        //drawData->AddBox( greenBox );
        //drawData->AddText( text );
        //ImGuiButton( "a" );

        //ImGuiText(  FormatFrameTime( Timestep::GetElapsedTime().mSeconds )  );

        FontApi::GetFontAtlasCell( Language::English, 'a', errors );
        FontApi::GetFontAtlasCell( Language::English, 'b', errors );



        Timestamp elapsedTime{ Timestep::GetElapsedTime() };
        float updateSpeed { 20 };
        float velocity { 20 };
        float radius { 100 };
        double t{ 1 / velocity * Floor( updateSpeed * elapsedTime.mSeconds ) };
        float x{ ( float )Cos(t) * radius + sWindowSize.x / 2 };
        float y{ ( float )Sin(t) * radius + sWindowSize.y / 2};

        //ImGuiIndent();
        for( int i {  }; i < 50; ++i )
        {
          ImGuiSetCursorPos( { x, y + 20 * i} );
          String str{ "text " + ToString( i ) };
          ImGuiText( str );
        }
        ImGuiEnd();
      }
    }


    if( mShowUnownedWindow )
    {
      ImGuiSetNextWindowPosition( v2( 500, 100 ) );
      ImGuiSetNextWindowSize( v2( 300, 300 ) );
      ImGuiSetNextWindowMoveResize();
      if( ImGuiBegin( "Owned Window" ) )
      {


        //ImGuiButton( "" );
        ImGuiEnd();
      }
    }

  }

} // namespace Tac

