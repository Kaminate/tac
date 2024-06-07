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


Tac::Creation     Tac::sCreation;

namespace Tac
{
  static WindowHandle sWindowHandle;
  static const v2i    sWindowSize( 800, 600 );

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
    const SysWindowApi windowApi { initParams.mWindowApi };

    const WindowCreateParams windowCreateParams
    {
      .mName { "level editor" },
      .mPos  { 50, 50 },
      .mSize { sWindowSize }, 
    };

    sWindowHandle = windowApi.CreateWindow( windowCreateParams, errors );
    sCreation.Init( errors );

    {
      //Render::ViewHandle2 viewHandle = Render::CreateView2();
    }
    ++asdf;

  }

  void LevelEditorApp::Update( UpdateParams updateParams, Errors& errors )
  {
    sCreation.Update( updateParams.mKeyboardApi, updateParams.mWindowApi, errors );
  }

  void LevelEditorApp::Uninit( Errors& errors )
  {
    sCreation.Uninit( errors );
  }

  void LevelEditorApp::Present( PresentParams presentParams, Errors& errors )
  {
     const SysWindowApi windowApi{ presentParams.mWindowApi };
    if( !windowApi.IsShown( sWindowHandle ) )
      return;

    const Render::SwapChainHandle swapChainHandle{
      windowApi.GetSwapChainHandle( sWindowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    TAC_CALL( renderDevice->Present( swapChainHandle, errors ) );
  }

  void LevelEditorApp::Render( RenderParams renderParams, Errors& errors )
  {
    const SysWindowApi windowApi{ renderParams.mWindowApi };
    if( !windowApi.IsShown( sWindowHandle ) )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::IContext::Scope renderContext{ renderDevice->CreateRenderContext( errors ) };
    const Render::SwapChainHandle swapChainHandle{ windowApi.GetSwapChainHandle( sWindowHandle ) };
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

  void                Creation::Init( Errors& )
  {
  }
  

  void                Creation::Uninit( Errors& )
  {
  }
  

  void                Creation::Update( const SimKeyboardApi keyboardApi,
                                        const SimWindowApi windowApi,
                                        Errors& )
  {
    mShowUnownedWindow = false;
    mShowOwnedWindow = true;

    if( mShowOwnedWindow )
    {
      ImGuiSetNextWindowHandle( sWindowHandle );
      if( ImGuiBegin( "Owned Window" ) )
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

        ImGuiText(  FormatFrameTime( Timestep::GetElapsedTime().mSeconds )  );

        
        const v2 mousePosScreenspace{ keyboardApi.GetMousePosScreenspace() };
        const v2i windowPos{ windowApi.GetPos( sWindowHandle ) };
        const v2i windowSize{ windowApi.GetSize( sWindowHandle ) };
        static v2 textBoxOffset{ 123, 40 };
        const v2i textBoxPos{ windowSize / 2 - textBoxOffset };
        
        ImGuiSetCursorPos( textBoxPos );
        ImGuiText( String() + "mouse pos: "
                   + ToString( mousePosScreenspace.x ) + ", "
                   + ToString( mousePosScreenspace.y ) );
        ImGuiSetCursorPos( textBoxPos + v2( 0, ImGuiGetFontSize() ) );
        ImGuiText( String() + "window pos: "
                   + ToString( windowPos.x ) + ", "
                   + ToString( windowPos.y ) );
        
        ImGuiSetCursorPos( textBoxPos + v2( 0, ImGuiGetFontSize() * 2 ) );
        ImGuiText( String() + "window size: "
                   + ToString( windowSize.x ) + ", "
                   + ToString( windowSize.y ) );

        //ImGuiSetCursorPos( textBoxPos + v2( 0, ImGuiGetFontSize() * 3 ) );
        //ImGuiDragFloat2( "offset", textBoxOffset.data() );

        FontApi::GetFontAtlasCell( Language::English, 'a' );
        FontApi::GetFontAtlasCell( Language::English, 'b' );



        Timestamp elapsedTime{ Timestep::GetElapsedTime() };
        float updateSpeed { 20 };
        float velocity { 20 };
        float radius { 100 };
        double t{ 1 / velocity * Floor( updateSpeed * elapsedTime.mSeconds ) };
        float x{ ( float )Cos(t) * radius + sWindowSize.x / 2 };
        float y{ ( float )Sin(t) * radius + sWindowSize.y / 2};

        //ImGuiIndent();
        for( int i {}; i < 0; ++i )
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
      ImGuiSetNextWindowPosition( v2( 1000, 100 ) );
      ImGuiSetNextWindowSize( v2( 300, 300 ) );
      ImGuiSetNextWindowMoveResize();
      if( ImGuiBegin( "Unowned Window" ) )
      {
        int i = (int) Timestep::GetElapsedTime().mSeconds;
        i %= 2;
        if( i > 0 )
          ImGuiText( "AAAA\nAAAA\nAAAA" );
        else
          ImGuiText( "BBBB BBBB BBBB" ); 

        ImGuiEnd();
      }
    }

  }

} // namespace Tac

