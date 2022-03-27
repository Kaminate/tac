#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_settings.h"
#include "src/game-examples/tac_example_fluid.h"
#include "src/game-examples/tac_examples.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

namespace Tac
{

  static DesktopWindowHandle   sDesktopWindowHandle;
  static DesktopWindowState    sSavedDesktopWindowState;

  static Vector< Example* >    sExamples;
  static int                   sExampleIndex;

  static void   ExamplesInitCallback( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Example.Window" );
    sExamples.push_back( new ExampleFluid );
    sExamples[ sExampleIndex ]->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void   ExamplesUninitCallback( Errors& errors )
  {
    sExamples[ sExampleIndex ]->Uninit( errors );
  }

  static void   ExamplesUpdateCallback( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    int offset = 0;
    int iSelected = -1;

    TAC_ASSERT( sExampleIndex >= 0 && sExampleIndex < sExamples.size() );

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "" );
    ImGuiText( String("Current Example: " ) + sExamples[ sExampleIndex ]->GetName() );
    offset -= ImGuiButton( "Prev" ) ? 1 : 0;
    ImGuiSameLine();
    offset += ImGuiButton( "Next" ) ? 1 : 0;
    if( ImGuiCollapsingHeader( "Select Example" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < sExamples.size(); ++i )
        if( ImGuiSelectable( sExamples[ i ]->GetName(), i == sExampleIndex ) )
          iSelected = i;
    }
    ImGuiEnd();

    Example* example = sExamples[ sExampleIndex ];
    int exampleIndexNext = ( sExampleIndex + sExamples.size() + offset ) % sExamples.size();
    exampleIndexNext = iSelected == -1 ? exampleIndexNext : iSelected;
    if( exampleIndexNext != sExampleIndex )
    {
      example->Uninit( errors );
      TAC_HANDLE_ERROR( errors );

      example = sExamples[ sExampleIndex = exampleIndexNext ];
      example->Init( errors );
      TAC_HANDLE_ERROR( errors );
    }


    example->Update( errors );


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
