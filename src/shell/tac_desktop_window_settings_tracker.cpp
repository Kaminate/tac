#include "src/common/containers/tac_vector.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_settings.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

namespace Tac
{
  struct TrackInfo
  {
    String              mPath;
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX;
    int                 mY;
    int                 mW;
    int                 mH;
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
    DesktopWindowHandle desktopWindowHandle = DesktopAppCreateWindow( x, y, w, h );
    DesktopAppMoveControls( desktopWindowHandle );
    DesktopAppResizeControls( desktopWindowHandle );
    TrackInfo info;
    info.mPath = path;
    info.mDesktopWindowHandle = desktopWindowHandle;
    info.mX = x;
    info.mY = y;
    info.mW = w;
    info.mH = h;
    sTrackInfos.push_back( info );
    return desktopWindowHandle;
  }

  void UpdateTrackedWindows()
  {
    for( TrackInfo& info : sTrackInfos )
    {
      DesktopWindowState* state = GetDesktopWindowState( info.mDesktopWindowHandle );
      if( !state || !state->mNativeWindowHandle )
        continue;
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

}
