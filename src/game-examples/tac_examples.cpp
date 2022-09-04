#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_settings.h"
#include "src/game-examples/tac_example_fluid.h"
#include "src/game-examples/tac_example_meta.h"
#include "src/game-examples/tac_example_phys_sim_force.h"
#include "src/game-examples/tac_examples.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

#include <set>

namespace Tac
{

  static DesktopWindowHandle   sDesktopWindowHandle;
  //static DesktopWindowState    sSavedDesktopWindowState;

  static Vector< Example* >    sExamples;
  static int                   sExampleIndex = -1;
  static int                   exampleIndexNext = -1;

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

    Example* curExample = GetExample( sExampleIndex );

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "" );
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

    if( curExample && offset )
      exampleIndexNext = ( sExampleIndex + sExamples.size() + offset ) % sExamples.size();

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
      curExample->Update( errors );


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
