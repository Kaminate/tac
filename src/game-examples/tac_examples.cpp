#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_camera.h"
#include "src/common/math/tac_math.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_frame_memory.h"
#include "src/common/tac_frame_memory.h"
#include "src/common/tac_os.h"
#include "src/common/tac_settings.h"
#include "src/game-examples/tac_examples.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

#include <set>

namespace Tac
{

  Example::Example()
  {
    mWorld = TAC_NEW World;
    mCamera = TAC_NEW Camera{ .mPos = { 0, 0, 5 },
                              .mForwards = { 0, 0, -1 },
                              .mRight = { 1, 0, 0 },
                              .mUp = { 0, 1, 0 } };
#if 0
    EntityUUIDCounter u;
    EntityUUID id = u.AllocateNewUUID();
    mEntity = mWorld->SpawnEntity( id );
    mModel = ( Model* )mEntity->AddNewComponent( Model().GetEntry() );
    mModel->mModelPath = "assets/editor/single_triangle.obj";
    mModel->mModelPath = "assets/essential/sphere.obj";
    mModel->mModelIndex = 0;
#endif
  }

  Example::~Example()
  {

    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }


  struct ExampleEntry
  {
    const char* GetName() { return mExampleName; }
    const char* mExampleName;
    Example*( * mExampleFactory )(); 
  };

  static DesktopWindowHandle       sDesktopWindowHandle;

  static Vector< ExampleEntry >    sExamples;
  static int                       sExampleIndexCurr = -1;
  static int                       sExampleIndexNext = -1;
  static Example*                  sCurrExample;

  static Render::ViewHandle        exampleView;
  static int                       exampleWidth;
  static int                       exampleHeight;
  static Render::TextureHandle     exampleColor;
  static Render::TextureHandle     exampleDepth;
  static Render::FramebufferHandle exampleFramebuffer;

  void ExampleRegistryAdd( const char* exampleName, Example*(* exampleFactory)() )
  {
    ExampleEntry exampleEntry;
    exampleEntry.mExampleName = exampleName;;
    exampleEntry.mExampleFactory = exampleFactory;
    sExamples.push_back( exampleEntry);
  }

  static void   VerifyExampleNames()
  {
    std::set< String > names;
    for( auto example : sExamples )
    {
      String name = example.mExampleName;
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
      if( sExamples[ i ].mExampleName == name )
        return i;
    return -1;
  }

  bool ExampleIndexValid( int i )
  {
    return i >= 0 && i < sExamples.size();
  }

  const ExampleEntry* GetExampleEntry( int i )
  {
    return ExampleIndexValid( i ) ? &sExamples[ i ] : nullptr;
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
    ExampleRegistryPopulate();
    VerifyExampleNames();

    const StringView settingExampleName = SettingsGetString( "Example.Name", "" );
    const int        settingExampleIndex = GetExampleIndex( settingExampleName );

    sExampleIndexNext = ExampleIndexValid( settingExampleIndex ) ? settingExampleIndex : 0;

    //// Init just the first example, sExmpleIndex = 0
    //sExamples[ sExampleIndex ]->Init( errors );
    //TAC_HANDLE_ERROR( errors );

    GamePresentationInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void   ExamplesUninitCallback( Errors& errors )
  {
    TAC_DELETE sCurrExample;
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

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "Examples" );
    if( sCurrExample )
    {
      ImGuiText( FrameMemoryPrintf( "Current Example: %s", sCurrExample->mName ) );
      offset -= ImGuiButton( "Prev" ) ? 1 : 0;
      ImGuiSameLine();
      offset += ImGuiButton( "Next" ) ? 1 : 0;
    }
    if( ImGuiCollapsingHeader( "Select Example" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < sExamples.size(); ++i )
        if( ImGuiSelectable( sExamples[ i ].GetName(), i == sExampleIndexCurr ) )
          iSelected = i;
    }

    int iOffset = ( sExampleIndexCurr + sExamples.size() + offset ) % sExamples.size();
    sExampleIndexNext = sCurrExample && offset ? iOffset : sExampleIndexNext;
    sExampleIndexNext = iSelected != -1 ? iSelected : sExampleIndexNext;

    const v2 cursorPos = ImGuiGetCursorPos();
    const UIStyle& style = ImGuiGetStyle();
    style.windowPadding;
    const v2 drawSize = {
      desktopWindowState->mWidth - style.windowPadding * 2,
      desktopWindowState->mHeight - style.windowPadding - cursorPos.y };
    
    ExamplesEnsureView(drawSize);

    if( ExampleIndexValid( sExampleIndexNext ) && sExampleIndexNext != sExampleIndexCurr )
    {
      if( sCurrExample )
      {
        TAC_DELETE sCurrExample;
        TAC_HANDLE_ERROR( errors );
      }

      sExampleIndexCurr = sExampleIndexNext;
      const ExampleEntry& entry = sExamples[ sExampleIndexCurr ];

      sCurrExample = entry.mExampleFactory();
      sCurrExample->mName = entry.mExampleName;
      TAC_HANDLE_ERROR( errors );

      SettingsSetString( "Example.Name", sCurrExample->mName );
    }

    if( sCurrExample )
    {
      sCurrExample->Update( errors );
      if( sCurrExample->mWorld && sCurrExample->mCamera && exampleView.IsValid() )
      {
        Render::SetViewport( exampleView, Render::Viewport( exampleWidth, exampleHeight ) );
        Render::SetViewScissorRect( exampleView, Render::ScissorRect( exampleWidth, exampleHeight ) );
        Render::SetViewFramebuffer( exampleView, exampleFramebuffer );
        GamePresentationRender( sCurrExample->mWorld,
                                sCurrExample->mCamera,
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

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 wsKeyboardForce{}; // worldspace keyboard force
    wsKeyboardForce += KeyboardIsKeyDown( Key::W ) ? mCamera->mUp : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::A ) ? -mCamera->mRight : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::S ) ? -mCamera->mUp : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::D ) ? mCamera->mRight : v3{};
    const float q = wsKeyboardForce.Quadrance();
    if( q )
      wsKeyboardForce /= Sqrt( q );
    return wsKeyboardForce;
  }

} // namespace Tac
