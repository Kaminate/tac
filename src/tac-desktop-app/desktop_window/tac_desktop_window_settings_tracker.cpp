#include "tac_desktop_window_settings_tracker.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
//#include "tac-engine-core/settings/tac_settings.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

namespace Tac
{
  static SimWindowApi* sWindowApi;
  static SettingsNode sSettingsNode;


  struct TrackInfo
  {
    String              mPath          {};
    WindowHandle        mWindowHandle  {};
    int                 mX             {};
    int                 mY             {};
    int                 mW             {};
    int                 mH             {};
    bool                mQuitOnClose   {};
    bool                mEverOpened    {};
    bool                mTrackSettings {};
  };

  static Vector< TrackInfo > sTrackInfos;

  static TrackInfo* FindTrackInfo( const WindowHandle& WindowHandle )
  {
    for( TrackInfo& info : sTrackInfos )
      if( info.mWindowHandle == WindowHandle )
        return &info;
    return nullptr;
  }


  static String GetJsonPath( StringView oldName )
  {
    String name { oldName };
    for( char& c : name )
      if( !IsAlpha( c ) && !IsDigit( c ) )
        c = '_';
    return name;
  }

  static void UpdateSettings( const StringView path,
                              int& x,
                              int& y,
                              int& w,
                              int& h )
  {
    const String jsonPath{ GetJsonPath( path ) };
    SettingsNode node{ sSettingsNode.GetChild( jsonPath ) };
    x = node.GetChild( "x" ).GetValueWithFallback( ( JsonNumber )x );
    y = node.GetChild( "y" ).GetValueWithFallback( ( JsonNumber )y );
    w = node.GetChild( "w" ).GetValueWithFallback( ( JsonNumber )w );
  }
}

Tac::WindowHandle Tac::CreateTrackedWindow( WindowCreateParams params )
{
 
  const WindowHandle windowHandle { sWindowApi->CreateWindow( params ) };
  //DesktopApp::GetInstance()->MoveControls( WindowHandle );
  //DesktopApp::GetInstance()->ResizeControls( WindowHandle );

  const char* path{ params.mName }; // just reuse it
  const TrackInfo info
  {
    .mPath          { path },
    .mWindowHandle  { windowHandle },
    .mX             { params.mPos.x },
    .mY             { params.mPos.y },
    .mW             { params.mSize.x },
    .mH             { params.mSize.y },
    .mTrackSettings { true },
  };
  sTrackInfos.push_back( info );
  return windowHandle;
}

Tac::WindowHandle Tac::CreateTrackedWindow( const StringView path,
                                            int x,
                                            int y,
                                            int w,
                                            int h )
{
  UpdateSettings( path, x, y, w, h );

  const WindowCreateParams createParams
  {
    .mName { path }, // reuse
    .mPos  { v2i( x, y ) },
    .mSize { v2i( w, h ) },
  };

  return CreateTrackedWindow( createParams );
}

void Tac::UpdateTrackedWindows()
{
  for( TrackInfo& info : sTrackInfos )
  {
    WindowHandle windowHandle { info.mWindowHandle };
    if( !sWindowApi->IsShown( windowHandle ) && info.mEverOpened )
      OS::OSAppStopRunning();

    if( !sWindowApi->IsShown( windowHandle ) )
      continue;

    info.mEverOpened = true;

    if( info.mTrackSettings )
    {
      const v2i pos { sWindowApi->GetPos( windowHandle ) };
      const v2i size { sWindowApi->GetSize( windowHandle ) };
      const int x { pos.x };
      const int y { pos.y };
      const int w { size.x };
      const int h { size.y };
      if( x == info.mX &&
          y == info.mY &&
          w == info.mW &&
          h == info.mH )
        continue;

      info.mX = x;
      info.mY = y;
      info.mW = w;
      info.mH = h;

      UpdateSettings( info.mPath, info.mX, info.mY, info.mW, info.mH );
    }
  }
}


void Tac::QuitProgramOnWindowClose( const WindowHandle& h )
{
  if( TrackInfo* trackInfo { FindTrackInfo( h ) } )
  {
    trackInfo->mQuitOnClose = true;
  }
  else
  {
    const TrackInfo info
    {
      .mWindowHandle { h },
      .mQuitOnClose { true },
    };
    sTrackInfos.push_back( info );
  }
}


void Tac::TrackWindowInit( SimWindowApi* windowApi, SettingsNode settingsNode )
{
  sWindowApi = windowApi;
  sSettingsNode = settingsNode;
}

