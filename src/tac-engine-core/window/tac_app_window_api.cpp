#include "tac_app_window_api.h"

#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_time.h"

namespace Tac
{
#if 1
  struct NextWindowData
  {
    bool                    mPositionValid;
    v2                      mPosition;
    //AppWindowMgr::Condition mPositionCondition;

    bool                    mSizeValid;
    v2                      mSize;
    //AppWindowMgr::Condition mSizeCondition;
  };


  struct AppWindowData
  {
    String       mName;
    WindowHandle mWindowHandle;
    GameTime     mRequestTime;
  };

  static NextWindowData        sNextWindow;
  static Vector<AppWindowData> sAllWindowDatas;
  static Vector<int>           sWindowDataStack;

  static auto FindAppWindowData( StringView name ) -> int
  {
    for( int i{}; i < sAllWindowDatas.size(); ++i ) // AppWindowData& data : sAllWindowDatas )
      if( ( StringView )sAllWindowDatas[ i ].mName == name )
        return i;
    return -1;
  }

  static auto GetWindowSettingsJson( const StringView& name ) -> SettingsNode
  {
    SettingsNode windowsJson{ Shell::sShellSettings.GetChild( "windowmgr.windows" ) };
    const int n{ windowsJson.GetValue().mArrayElements.size() };
    for( int i{}; i < n; ++i )
      if( SettingsNode child{ windowsJson.GetChild( "[" + ToString( i ) + "]" ) };
          ( StringView )child.GetChild( "name" ).GetValue().mString == name )
        return child;

    SettingsNode child{ windowsJson.GetChild( "[" + ToString( n ) + "]" ) };
    child.GetChild( "name" ).SetValue( name );
    return child;
  }

  void AppWindowMgr::SetNextWindowPosition( v2 pos/*, Condition cond*/ )
  {
    sNextWindow.mPosition = pos;
    sNextWindow.mPositionValid = true;
    //sNextWindow.mPositionCondition = cond;
  }

  void AppWindowMgr::SetNextWindowSize( v2 size/*, Condition cond*/ )
  {
    sNextWindow.mSize = size;
    sNextWindow.mSizeValid = true;
    //sNextWindow.mSizeCondition = cond;
  }

  bool AppWindowMgr::WindowBegin( StringView name )
  {
    int iData{ FindAppWindowData( name ) };
    if( iData == -1 )
    {
      int x = 50;
      int y = 50;
      int w = 400;
      int h = 300;

      SettingsNode windowJson{ GetWindowSettingsJson( name ) };
      x = ( int )windowJson.GetChild( "x" ).GetValueWithFallback( ( JsonNumber )x ).mNumber;
      y = ( int )windowJson.GetChild( "y" ).GetValueWithFallback( ( JsonNumber )y ).mNumber;
      w = ( int )windowJson.GetChild( "w" ).GetValueWithFallback( ( JsonNumber )w ).mNumber;
      h = ( int )windowJson.GetChild( "h" ).GetValueWithFallback( ( JsonNumber )h ).mNumber;

      if( sNextWindow.mPositionValid )
      {
        x = (int)sNextWindow.mPosition.x;
        y = (int)sNextWindow.mPosition.y;
      }

      if( sNextWindow.mSizeValid )
      {
        w = (int)sNextWindow.mSize.x;
        h = (int)sNextWindow.mSize.y;
      }

      static Errors errors;
      WindowHandle windowHandle{
      AppWindowApi::CreateWindow(
        WindowCreateParams
        {
          .mName { name },
          .mPos  { x, y },
          .mSize { w, h },
        }, errors ) };
      TAC_ASSERT( !errors );

      iData = sAllWindowDatas.size();
      sAllWindowDatas.push_back(
        AppWindowData
        {
          .mName         { name },
          .mWindowHandle { windowHandle },
        } );
    }

    sNextWindow = {};
    AppWindowData* data{ &sAllWindowDatas[ iData ] };
    data->mRequestTime = GameTimer::GetElapsedTime();
    sWindowDataStack.push_back( iData );
    return AppWindowApi::IsShown( data->mWindowHandle );
  }

  auto AppWindowMgr::GetWindowHandle() -> WindowHandle
  {
    return sAllWindowDatas[ sWindowDataStack.back() ].mWindowHandle;
  }
  void AppWindowMgr::WindowEnd()
  {
    sWindowDataStack.pop_back();
  }
  void AppWindowMgr::FrameBegin()
  {
    TAC_ASSERT( sWindowDataStack.empty() );
  }
  void AppWindowMgr::FrameEnd()
  {
    TAC_ASSERT( sWindowDataStack.empty() );
    int iData{};
    int nData{ sAllWindowDatas.size() };
    const GameTime curTime{ GameTimer::GetElapsedTime() };
    while( iData < nData )
    {
      AppWindowData* data{ &sAllWindowDatas[ iData ] };
      if( ( curTime - data->mRequestTime ) > GameTimeDelta{ .mSeconds{ 0.1f } } )
      {
        AppWindowApi::DestroyWindow( data->mWindowHandle );
        Swap( sAllWindowDatas[ iData ], sAllWindowDatas[ --nData ] );
      }
      else
      {
        ++iData;
      }
    }
    sAllWindowDatas.resize( nData );
  }

  auto AppWindowMgr::GetWindowHandle( StringView name ) -> WindowHandle
  {
    const int iData{ FindAppWindowData( name ) };
    return iData == -1 ? WindowHandle{} : sAllWindowDatas[ iData ].mWindowHandle;
  }

  void AppWindowMgr::RenderPresent( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    for( AppWindowData& appWindowData : sAllWindowDatas )
    {
      if( const WindowHandle windowHandle{ appWindowData.mWindowHandle };
          AppWindowApi::IsShown( windowHandle ) )
      {
        const Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( windowHandle ) };
        TAC_CALL( renderDevice->Present( swapChain, errors ) );
      }

    }
  }
#endif
} // namespace Tac


