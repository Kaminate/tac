#include "tac_desktop_window_settings_tracker.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/window/tac_window_api.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/tac_desktop_app.h"

namespace Tac
{
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

Tac::WindowHandle Tac::CreateTrackedWindow( const WindowApi::CreateParams& params )
{
  return CreateTrackedWindow( params.mName,
                              params.mX,
                              params.mY,
                              params.mWidth,
                              params.mHeight );
}

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
  const WindowApi::CreateParams createParams
  {
    .mName = path,
    .mX = x,
    .mY = y,
    .mWidth = w,
    .mHeight = h,
  };

  const WindowHandle windowHandle = WindowApi::CreateWindow( createParams );
  //DesktopApp::GetInstance()->MoveControls( WindowHandle );
  //DesktopApp::GetInstance()->ResizeControls( WindowHandle );

  const TrackInfo info
  {
    .mPath = path,
    .mWindowHandle = windowHandle,
    .mX = x,
    .mY = y,
    .mW = w,
    .mH = h,
  };
  sTrackInfos.push_back( info );
  return windowHandle;
}

void Tac::UpdateTrackedWindows()
{
  for( TrackInfo& info : sTrackInfos )
  {
    WindowHandle windowHandle = info.mWindowHandle;
    if( !windowHandle.IsShown() && info.mEverOpened )
      OS::OSAppStopRunning();

    if( !windowHandle.IsShown() )
      continue;

    info.mEverOpened = true;

    const int x = windowHandle.GetX();
    const int y = windowHandle.GetY();
    const int w = windowHandle.GetWidth();
    const int h = windowHandle.GetHeight();
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

