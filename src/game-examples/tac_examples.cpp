#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_os.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_settings.h"
#include "src/common/tac_frame_memory.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/game-examples/tac_example_fluid.h"
#include "src/game-examples/tac_example_meta.h"
#include "src/game-examples/tac_example_phys_sim_force.h"
#include "src/game-examples/tac_examples.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

#include <set>

namespace Tac
{

  static DesktopWindowHandle       sDesktopWindowHandle;

  static Vector< Example* >        sExamples;
  static int                       sExampleIndex = -1;
  static int                       exampleIndexNext = -1;

  static Render::ViewHandle        exampleView;
  static int                       exampleWidth;
  static int                       exampleHeight;
  static Render::TextureHandle     exampleColor;
  static Render::TextureHandle     exampleDepth;
  static Render::FramebufferHandle exampleFramebuffer;

  static void   VerifyExampleNames()
  {
    std::set< String > names;
    for( Example* example : sExamples )
    {
      String name = example->GetName();
      TAC_ASSERT( !name.empty() );

      TAC_ASSERT( !names.contains( name ) );

      // Check for copy paste errors
      //TAC_ASSERT( names.find( name ) == names.end() );
      names.insert( name );
    }
  }

  int GetExampleIndex( StringView name )
  {
    for( int i = 0; i < sExamples.size(); ++i )
      if( sExamples[ i ]->GetName() == name )
        return i;
    return -1;
  }

  bool ExampleIndexValid( int i )
  {
    return i >= 0 && i < sExamples.size();
  }

  Example* GetExample( int i )
  {
    return ExampleIndexValid( i ) ? sExamples[ i ] : nullptr;
  }

  static void   ExamplesEnsureView(v2 size)
  {
    const int w = (int)size.x;
    const int h = (int)size.y;
    const bool shouldCreate = w * h > 0 && ( w != exampleWidth || h != exampleHeight );
    if( !shouldCreate )
      return;



      const char* debugName = "examplesview";
      const Render::TexSpec texSpecColor =
      {
        .mImage =
        {
          .mWidth = w,
          .mHeight = h,
          .mFormat =
          {
            .mElementCount = 4,
            .mPerElementByteCount = 1,
            .mPerElementDataType = Render::GraphicsType::unorm,
          },
        },
        .mBinding = Render::Binding::ShaderResource | Render::Binding::RenderTarget,
      };
      const Render::TextureHandle textureHandleColor = Render::CreateTexture( texSpecColor, TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( textureHandleColor, debugName );

      const Render::TexSpec texSpecDepth = 
      {
        .mImage =
        {
          .mWidth = w,
          .mHeight = h,
          .mFormat =
          {
            .mElementCount = 1,
            .mPerElementByteCount = sizeof( uint16_t ),
            .mPerElementDataType = Render::GraphicsType::unorm,
          },
        },
        .mBinding = Render::Binding::DepthStencil,
      };
      const Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( textureHandleDepth, debugName );

      const Render::FramebufferHandle framebufferHandle = Render::CreateFramebufferForRenderToTexture(
        { textureHandleColor, textureHandleDepth }, TAC_STACK_FRAME );
      const Render::ViewHandle viewHandle = Render::CreateView();

    Render::DestroyView(exampleView);
    Render::DestroyTexture(exampleColor, TAC_STACK_FRAME);
    Render::DestroyTexture(exampleDepth, TAC_STACK_FRAME);
    Render::DestroyFramebuffer(exampleFramebuffer, TAC_STACK_FRAME);

    exampleView = viewHandle;
    exampleColor = textureHandleColor;
    exampleDepth = textureHandleDepth;
    exampleFramebuffer = framebufferHandle;
    exampleWidth = w;
    exampleHeight = h;
  }


  static void   ExamplesInitCallback( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Example.Window" );
    sExamples.push_back( new ExampleFluid );
    sExamples.push_back( new ExampleMeta );
    sExamples.push_back( new ExamplePhysSimForce );
    VerifyExampleNames();

    const StringView settingExampleName = SettingsGetString( "Example.Name", "" );
    const int        settingExampleIndex = GetExampleIndex( settingExampleName );

    exampleIndexNext = ExampleIndexValid( settingExampleIndex ) ? settingExampleIndex : 0;

    //// Init just the first example, sExmpleIndex = 0
    //sExamples[ sExampleIndex ]->Init( errors );
    //TAC_HANDLE_ERROR( errors );

    GamePresentationInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void   ExamplesUninitCallback( Errors& errors )
  {
    sExamples[ sExampleIndex ]->Uninit( errors );
  }

  static void   ExamplesQuitOnWindowClose()
  {
    static bool windowEverOpened;
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( desktopWindowState->mNativeWindowHandle )
    {
      windowEverOpened = true;
    }
    else
    {
      if( windowEverOpened )
      {
        GetOS()->OSAppStopRunning();
      }
    }
  }

  static void   ExamplesUpdateCallback( Errors& errors )
  {
    ExamplesQuitOnWindowClose();

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    int offset = 0;
    int iSelected = -1;

    Example* curExample = GetExample( sExampleIndex );

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "Examples" );
    if( curExample )
    {

      ImGuiText( String( "Current Example: " ) + curExample->GetName() );
      offset -= ImGuiButton( "Prev" ) ? 1 : 0;
      ImGuiSameLine();
      offset += ImGuiButton( "Next" ) ? 1 : 0;
    }
    if( ImGuiCollapsingHeader( "Select Example" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < sExamples.size(); ++i )
        if( ImGuiSelectable( sExamples[ i ]->GetName(), i == sExampleIndex ) )
          iSelected = i;
    }

    int iOffset = ( sExampleIndex + sExamples.size() + offset ) % sExamples.size();
    exampleIndexNext = curExample && offset ? iOffset : exampleIndexNext;
    exampleIndexNext = iSelected != -1 ? iSelected : exampleIndexNext;

    const v2 cursorPos = ImGuiGetCursorPos();
    const UIStyle& style = ImGuiGetStyle();
    style.windowPadding;
    const v2 drawSize = {
      desktopWindowState->mWidth - style.windowPadding * 2,
      desktopWindowState->mHeight - style.windowPadding - cursorPos.y };
    
    ExamplesEnsureView(drawSize);

    if( ExampleIndexValid( exampleIndexNext ) && exampleIndexNext != sExampleIndex )
    {
      if( curExample )
      {
        curExample->Uninit( errors );
        TAC_HANDLE_ERROR( errors );
      }

      curExample = GetExample( sExampleIndex = exampleIndexNext );
      curExample->Init( errors );
      TAC_HANDLE_ERROR( errors );

      SettingsSetString( "Example.Name", curExample->GetName() );
    }

    if( curExample )
    {
      curExample->Update( errors );
      if( curExample->mWorld && curExample->mCamera && exampleView.IsValid() )
      {
        Render::SetViewport( exampleView, Render::Viewport( exampleWidth, exampleHeight ) );
        Render::SetViewScissorRect( exampleView, Render::ScissorRect( exampleWidth, exampleHeight ) );
        Render::SetViewFramebuffer( exampleView, exampleFramebuffer );
        GamePresentationRender( curExample->mWorld,
                                curExample->mCamera,
                                exampleWidth,
                                exampleHeight,
                                exampleView );
      }

      ImGuiImage( (int)exampleColor, drawSize );
    }


    ImGuiEnd();
    TAC_HANDLE_ERROR( errors );
  }

  void          ExecutableStartupInfo::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mAppName = "Examples";
    mProjectInit = ExamplesInitCallback;
    mProjectUpdate = ExamplesUpdateCallback;
    mProjectUninit = ExamplesUninitCallback;
  }

} // namespace Tac
