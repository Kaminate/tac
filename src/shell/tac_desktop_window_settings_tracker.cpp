#include "src/common/containers/tac_vector.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

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
  

  DesktopWindowHandle CreateTrackedWindow( const StringView& path,
                                           int x,
                                           int y,
                                           int w,
                                           int h )
  {
    Json* json = SettingsGetJson( path );
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

    const DesktopWindowHandle desktopWindowHandle = DesktopAppCreateWindow( createParams );
    DesktopAppMoveControls( desktopWindowHandle );
    DesktopAppResizeControls( desktopWindowHandle );

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
      DesktopWindowState* state = GetDesktopWindowState( info.mDesktopWindowHandle );
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
