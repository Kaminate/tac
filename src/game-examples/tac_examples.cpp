#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/math/tac_math.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/system/tac_os.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/game-examples/tac_examples.h"
#include "src/game-examples/tac_examples_state_machine.h"
#include "src/game-examples/tac_examples_presentation.h"
#include "src/game-examples/tac_examples_registry.h"
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
  }

  Example::~Example()
  {
    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }


  static DesktopWindowHandle       sDesktopWindowHandle;

  //const ExampleEntry* GetExampleEntry( int i )
  //{
  //  return ExampleIndexValid( i ) ? &sExamples[ i ] : nullptr;
  //}


  static void   ExamplesInitCallback( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Example.Window" );
    QuitProgramOnWindowClose( sDesktopWindowHandle );
    ExampleRegistryPopulate();

    const StringView settingExampleName = SettingsGetString( "Example.Name", "" );
    const int        settingExampleIndex = GetExampleIndex( settingExampleName );

    SetNextExample(settingExampleIndex);

    GamePresentationInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void   ExamplesUninitCallback( Errors& errors )
  {
    ExampleStateMachineUnint();
  }

  //static void   ExamplesQuitOnWindowClose()
  //{
  //  static bool windowEverOpened;
  //  DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
  //  if( desktopWindowState->mNativeWindowHandle )
  //  {
  //    windowEverOpened = true;
  //  }
  //  else
  //  {
  //    if( windowEverOpened )
  //    {
  //      OS::OSAppStopRunning();
  //    }
  //  }
  //}

  static void   ExamplesUpdateCallback( Errors& errors )
  {
    //ExamplesQuitOnWindowClose();

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    int offset = 0;
    int iSelected = -1;

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "Examples" );

    if( Example* ex = GetCurrExample() )
    {
      ImGuiText( FrameMemoryPrintf( "Current Example: %s", ex->mName ) );
      offset -= ImGuiButton( "Prev" ) ? 1 : 0;
      ImGuiSameLine();
      offset += ImGuiButton( "Next" ) ? 1 : 0;
    }

    if( ImGuiCollapsingHeader( "Select Example" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < GetExampleCount(); ++i )
        if( ImGuiSelectable( GetExampleName(i), i == GetCurrExampleIndex() ) )
          iSelected = i;
    }

    if(offset)
      SetNextExample( GetCurrExampleIndex() + offset );
    if(iSelected != -1)
      SetNextExample( iSelected );

    const v2 cursorPos = ImGuiGetCursorPos();
    const UIStyle& style = ImGuiGetStyle();
    style.windowPadding;
    const v2 drawSize = {
      desktopWindowState->mWidth - style.windowPadding * 2,
      desktopWindowState->mHeight - style.windowPadding - cursorPos.y };
    

    const int iOld = GetCurrExampleIndex();
    ExampleStateMachineUpdate(errors);
    TAC_HANDLE_ERROR(errors);
    const int iNew = GetCurrExampleIndex();
    if( iOld != iNew )
      SettingsSetString( "Example.Name", GetExampleName( iNew ) );

    if( Example* ex = GetCurrExample() )
    {
      ExamplesPresentationRender( ex->mWorld, ex->mCamera, drawSize );
      ImGuiImage( ( int )ExamplesColor(), drawSize );
    }


    ImGuiEnd();
    TAC_HANDLE_ERROR( errors );
  }

  ExecutableStartupInfo          ExecutableStartupInfo::Init()
  {
    return { .mAppName = "Examples",
             .mProjectInit = ExamplesInitCallback,
             .mProjectUpdate = ExamplesUpdateCallback,
             .mProjectUninit = ExamplesUninitCallback, };
  }

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 wsKeyboardForce{};
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
