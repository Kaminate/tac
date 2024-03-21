#include "tac_desktop_window_settings_tracker.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/tac_desktop_app.h"

namespace Tac
{
  static SimWindowApi* sWindowApi;
  struct TrackInfo
  {
    String              mPath;
    WindowHandle mWindowHandle;
    int                 mX = 0;
    int                 mY = 0;
    int                 mW = 0;
    int                 mH = 0;
    bool                mQuitOnClose = false;
    bool                mEverOpened = false;
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
    String name = oldName;
    for( char& c : name )
      if( !IsAlpha( c ) && !IsDigit( c ) )
        c = '_';
    return name;

  }
}

Tac::WindowHandle Tac::CreateTrackedWindow( const SimWindowApi::CreateParams& params )
{
 
  const WindowHandle windowHandle = sWindowApi->CreateWindow( params );
  //DesktopApp::GetInstance()->MoveControls( WindowHandle );
  //DesktopApp::GetInstance()->ResizeControls( WindowHandle );

  const char* path = params.mName; // just reuse it
  const TrackInfo info
  {
    .mPath = path,
    .mWindowHandle = windowHandle,
    .mX = params.mPos.x,
    .mY = params.mPos.y,
    .mW = params.mSize.x,
    .mH = params.mSize.y,
  };
  sTrackInfos.push_back( info );
  return windowHandle;}

Tac::WindowHandle Tac::CreateTrackedWindow( const StringView& path,
                                            int x,
                                            int y,
                                            int w,
                                            int h )
{
  String jsonPath = GetJsonPath( path );
  Json* json = SettingsGetJson( jsonPath );
  x = ( int )SettingsGetNumber( "x", x, json );
  y = ( int )SettingsGetNumber( "y", y, json );
  w = ( int )SettingsGetNumber( "w", w, json );
  h = ( int )SettingsGetNumber( "h", h, json );
  const char* name = path; // just reuse it

  //const WindowApi::CreateParams createParams
  const SimWindowApi::CreateParams createParams
  {
    .mName = path,
    .mPos = v2i( x, y ),
    .mSize = v2i( w, h ),
  };

  return CreateTrackedWindow( createParams );
}

void Tac::UpdateTrackedWindows()
{
  for( TrackInfo& info : sTrackInfos )
  {
    WindowHandle windowHandle = info.mWindowHandle;
    if( !sWindowApi-> IsShown(windowHandle) && info.mEverOpened )
      OS::OSAppStopRunning();

    if( !sWindowApi->IsShown(windowHandle) )
      continue;

    info.mEverOpened = true;

    const v2i pos = sWindowApi->GetPos( windowHandle );
    const v2i size = sWindowApi->GetSize( windowHandle );
    const int x = pos.x;
    const int y = pos.y;
    const int w = size.x;
    const int h = size.y;
    if( x == info.mX &&
        y == info.mY &&
        w == info.mW &&
        h == info.mH )
      continue;

    info.mX = x;
    info.mY = y;
    info.mW = w;
    info.mH = h;
    Json* json = SettingsGetJson( info.mPath );
    SettingsSetNumber( "x", info.mX, json );
    SettingsSetNumber( "y", info.mY, json );
    SettingsSetNumber( "w", info.mW, json );
    SettingsSetNumber( "h", info.mH, json );
  }
}


void Tac::QuitProgramOnWindowClose( const WindowHandle& WindowHandle )
{
  TrackInfo* trackInfo = FindTrackInfo( WindowHandle );
  TAC_ASSERT( trackInfo ); // maybe todo make this work on non-tracked windows?
  trackInfo->mQuitOnClose = true;
}


void Tac::TrackWindowInit( SimWindowApi* windowApi )
{
  sWindowApi = windowApi;
}

