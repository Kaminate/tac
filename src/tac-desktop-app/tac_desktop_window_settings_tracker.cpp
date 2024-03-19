#include "tac_desktop_window_settings_tracker.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/system/tac_desktop_window.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-desktop-app/tac_desktop_app.h"

namespace Tac
{
  struct TrackInfo
  {
    String              mPath;
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX = 0;
    int                 mY = 0;
    int                 mW = 0;
    int                 mH = 0;
    bool                mQuitOnClose = false;
    bool                mEverOpened = false;
  };

  static Vector< TrackInfo > sTrackInfos;

  static String GetJsonPath( StringView oldName )
  {
    String name = oldName;
    for( char& c : name )
      if( !IsAlpha(c ) && !IsDigit(c) )
        c = '_';
    return name;
  }
  
  DesktopWindowHandle CreateTrackedWindow( const DesktopAppCreateWindowParams& params )
  {
    return CreateTrackedWindow( params.mName,
                                params.mX,
                                params.mY,
                                params.mWidth,
                                params.mHeight );
  }

  DesktopWindowHandle CreateTrackedWindow( const StringView& path,
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

    const DesktopAppCreateWindowParams createParams
    {
      .mName = path,
      .mX = x,
      .mY = y,
      .mWidth = w,
      .mHeight = h,
    };

    const DesktopWindowHandle desktopWindowHandle = DesktopApp::GetInstance()->CreateWindow( createParams );
    DesktopApp::GetInstance()->MoveControls( desktopWindowHandle );
    DesktopApp::GetInstance()->ResizeControls( desktopWindowHandle );

    const TrackInfo info
    {
      .mPath = path,
      .mDesktopWindowHandle = desktopWindowHandle,
      .mX = x,
      .mY = y,
      .mW = w,
      .mH = h,
    };
    sTrackInfos.push_back( info );
    return desktopWindowHandle;
  }

  void UpdateTrackedWindows()
  {
    for( TrackInfo& info : sTrackInfos )
    {
      DesktopWindowState* state = info.mDesktopWindowHandle.GetDesktopWindowState();
      if( !state )
        continue;

      if( !state->mNativeWindowHandle )
      {
        if( info.mEverOpened && info.mQuitOnClose )
        {
          OS::OSAppStopRunning();
        }

        continue;
      }

      info.mEverOpened = true;

      if( state->mX == info.mX &&
          state->mY == info.mY &&
          state->mWidth == info.mW &&
          state->mHeight == info.mH )
        continue;

      info.mX = state->mX;
      info.mY = state->mY;
      info.mW = state->mWidth;
      info.mH = state->mHeight;
      Json* json = SettingsGetJson( info.mPath );
      SettingsSetNumber( "x", info.mX, json );
      SettingsSetNumber( "y", info.mY, json );
      SettingsSetNumber( "w", info.mW, json );
      SettingsSetNumber( "h", info.mH, json );
    }
  }

  static TrackInfo* FindTrackInfo( const DesktopWindowHandle& desktopWindowHandle )
  {
    for( TrackInfo& info : sTrackInfos )
      if( info.mDesktopWindowHandle == desktopWindowHandle )
        return &info;
    return nullptr;
  }

  void QuitProgramOnWindowClose( const DesktopWindowHandle& desktopWindowHandle )
  {
    TrackInfo* trackInfo = FindTrackInfo( desktopWindowHandle );
    TAC_ASSERT( trackInfo ); // maybe todo make this work on non-tracked windows?
    trackInfo->mQuitOnClose = true;
  }

} // namespace Tac