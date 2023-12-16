#include "src/shell/sdl/tacsdllib/tac_sdl_app.h"
#include "src/common/tac_os.h"
#include "src/common/tac_id_collection.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_utility.h"

#include <SDL_syswm.h> // system window manager
#include <SDL_rwops.h> // file i/o abstraction, read write operations

namespace Tac
{

  static const int    kSDLSemaphoreCapacity = 10;
  static IdCollection sSDLSemaphoreIds( kSDLSemaphoreCapacity );
  static SDL_sem*     sSDLSemaphores[ kSDLSemaphoreCapacity ];



  //
  //  SDLWindow::~SDLWindow()
  //  {
  //    SDL_DestroyWindow( mWindow );
  //    SDLApp::Instance->mWindows.erase( this );
  //  }
  //  void* SDLWindow::GetOperatingSystemHandle()
  //  {
  //    return mOperatingSystemHandle;
  //  }
  //
  //  SDLApp* SDLApp::Instance;
  //  SDLApp::SDLApp()
  //  {
  //    Instance = this;
  //  }
  //  SDLApp::~SDLApp()
  //  {
  //  }
  //  void SDLApp::Init( Errors& errors )
  //  {
  //  }
  //  void SDLApp::Poll( Errors& errors )
  //  {
  //    SDL_Event event;
  //    while( SDL_PollEvent( &event ) )
  //    {
  //      if( OSmShouldStopRunning )
  //        break;
  //      switch( event.type )
  //      {
  //        case SDL_QUIT:
  //        {
  //          OSmShouldStopRunning = true;
  //        } break;
  //        case SDL_WINDOWEVENT:
  //        {
  //          SDLWindow* sdlWindow = FindSDLWindowByID( event.window.windowID );
  //          switch( event.window.event )
  //          {
  //            case SDL_WINDOWEVENT_CLOSE:
  //            {
  //              delete sdlWindow;
  //            } break;
  //            case SDL_WINDOWEVENT_RESIZED:
  //            {
  //              sdlWindow->mWidth = ( int )event.window.data1;
  //              sdlWindow->mHeight = ( int )event.window.data2;
  //              //sdlWindow->mRendererData->OnResize( errors );
  //            } break;
  //          }
  //        } break;
  //      }
  //    }
  //  }
  //  void SDLApp::GetPrimaryMonitor( Monitor* monitor, Errors& errors )
  //  {
  //    SDL_Rect rect;
  //    if( SDL_GetDisplayBounds( 0, &rect ) )
  //    {
  //      errors = va( "Failed to get display bounds %s", SDL_GetError() );
  //      TAC_HANDLE_ERROR( errors );
  //    }
  //    monitor->w = rect.w;
  //    monitor->h = rect.h;
  //  }
  //  SDLWindow* SDLApp::FindSDLWindowByID( Uint32 windowID )
  //  {
  //    for( SDLWindow* linuxWindow : mWindows )
  //    {
  //      if( SDL_GetWindowID( linuxWindow->mWindow ) == windowID )
  //      {
  //        return linuxWindow;
  //      }
  //    }
  //    return nullptr;
  //  }
  //  void SDLApp::SpawnWindow( DesktopWindowHandle handle,
  //                            int x,
  //                            int y,
  //                            int width,
  //                            int height )
  //  {
  //    Uint32 flags =
  //      SDL_WINDOW_SHOWN |
  //      SDL_WINDOW_RESIZABLE |
  //      //SDL_WINDOW_BORDERLESS |
  //      0;
  //    SDL_Window* sdlWindow = SDL_CreateWindow( "Tac",
  //                                              x,
  //                                              y,
  //                                              width,
  //                                              height,
  //                                              flags );
  //    SDL_RaiseWindow( sdlWindow );
  //
  //    void* operatingSystemHandle = nullptr;
  //    void* operatingSystemApplicationHandle = nullptr;
  //#if defined(SDL_VIDEO_DRIVER_WINDOWS)
  //    SDL_SysWMinfo wmInfo;
  //    SDL_VERSION( &wmInfo.version );
  //    if( SDL_FALSE == SDL_GetWindowWMInfo( sdlWindow, &wmInfo ) )
  //    {
  //      TAC_INVALID_CODE_PATH;
  //      //errors = "Failed to get sdl window wm info";
  //      //TAC_HANDLE_ERROR( errors );
  //    }
  //    operatingSystemHandle = wmInfo.info.win.window;
  //    operatingSystemApplicationHandle = wmInfo.info.win.hinstance;
  //#endif
  //
  //    auto linuxWindow = new SDLWindow();
  //    linuxWindow->mWindow = sdlWindow;
  //    linuxWindow->mOperatingSystemHandle = operatingSystemHandle;
  //    linuxWindow->mOperatingSystemApplicationHandle = operatingSystemApplicationHandle;
  //    //*( WindowParams* )linuxWindow = windowParams;
  //    //*desktopWindow = linuxWindow;
  //    //mWindows.insert( linuxWindow );
  //  }

  void                SDLProjectInit( Errors& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLProjectUpdate( Errors& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLProjectUninit( Errors& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformFrameBegin( Errors& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformFrameEnd( Errors& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformSpawnWindow( const DesktopWindowHandle&,
                                              int x,
                                              int y,
                                              int width,
                                              int height )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformDespawnWindow( const DesktopWindowHandle& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformWindowMoveControls( const DesktopWindowHandle&, const DesktopWindowRect& )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void                SDLPlatformWindowResizeControls( const DesktopWindowHandle&, int )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  DesktopWindowHandle SDLPlatformGetMouseHoveredWindow()
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
    return DesktopWindowHandle();
  }


  void            SDLOSDebugBreak() 
  {
    SDL_TriggerBreakpoint();
  }

  void            SDLOSDebugPopupBox( const StringView& s ) 
  {
    SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Hello", s, nullptr );
  }

  Filesystem::Path            SDLOSGetApplicationDataPath( Errors& errors ) 
  {
    //ExecutableStartupInfo info = ExecutableStartupInfo::Init();
    ExecutableStartupInfo info = ExecutableStartupInfo::sInstance;
    String org = info.mStudioName;
    String app = info.mAppName;
    TAC_ASSERT( !org.empty() && !app.empty() );
    Filesystem::Path path = SDL_GetPrefPath( org, app );
    return path;
  }

  void            SDLOSGetFileLastModifiedTime( std::time_t*, StringView path, Errors& ) 
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void            SDLOSGetFilesInDirectoryAux( Vector< String >& files, const std::filesystem::directory_entry& entry )
  {
    if( entry.is_regular_file() )
    {
      String subDirPath = entry.path().string().c_str();
      files.push_back( subDirPath );
    }
  }

  void            SDLOSGetFilesInDirectory( Vector< String >& files,
                                         StringView dir,
                                         OSGetFilesInDirectoryFlags flags,
                                         Errors& ) 
  {
    std::filesystem::path dirpath = dir.c_str();
    std::filesystem::recursive_directory_iterator itRecurse( dirpath );
    std::filesystem::directory_iterator itNonRecurse( dirpath );
    const bool recurse = ( int )flags & ( int )OSGetFilesInDirectoryFlags::Recursive;
    if( recurse )
    {
      for( const std::filesystem::directory_entry& entry : itRecurse )
      {
        SDLOSGetFilesInDirectoryAux( files, entry );
      }
    }
    else
    {
      for( const std::filesystem::directory_entry& entry : itNonRecurse )
      {
        SDLOSGetFilesInDirectoryAux( files, entry );
      }
    }
  }

  void            SDLOSGetDirectoriesInDirectory( Vector< String >& dirs, StringView dir, Errors& ) 
  {
    std::filesystem::path dirpath = dir.c_str();
    for( const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator( dirpath ) )
    {
      if( entry.is_directory() )
      {
        String subDirPath = entry.path().string().c_str();
        dirs.push_back( subDirPath );
      }
    }
  }

  void            SDLOSSaveDialog( String& path, StringView suggestedPath, Errors& ) 
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }

  void            SDLOSOpenDialog( String& path, Errors& ) 
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
  }


  void            SDLOSGetPrimaryMonitor( int* w, int* h ) 
  {
    int maxArea = 0;
    int maxAreaW = 0;
    int maxAreaH = 0;
    for( int iDisplay = 0; iDisplay < SDL_GetNumVideoDisplays(); ++iDisplay )
    {
      SDL_DisplayMode mode;
      int displayResult = SDL_GetCurrentDisplayMode( iDisplay, &mode );
      if( displayResult )
        continue; // todo use SDL_GetError()
      int area = mode.w * mode.h;
      if( area <= maxArea )
        continue;
      maxArea = area;
      maxAreaW = mode.w;
      maxAreaH = mode.h;
    }
    *w = maxAreaW;
    *h = maxAreaH;
  }

  //void        OSSetScreenspaceCursorPos( v2& pos, Errors& ) 
  //{
  //  int x;
  //  int y;
  //  SDL_GetGlobalMouseState( &x, &y );
  //  pos.x = x;
  //  pos.y = y;
  //}

  void            SDLOSSetScreenspaceCursorPos( const v2& pos, Errors& errors ) 
  {
    TAC_RAISE_ERROR_IF( SDL_WarpMouseGlobal( ( int )pos.x, ( int )pos.y ) < 0, SDL_GetError(), errors );
  }


  void* SDLOSGetLoadedDLL(StringView name)
  {
    // in windows, you can load a dll with ::LoadLibraryA, 
    // and check if it has already been loaded with ::GetModuleHandleA
    //
    // in sdl, you can load a dll with SDL_LoadObject ( i think ),
    // but i dont see any way to check if its already been loaded
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
    return nullptr;
  }

  void* SDLOSLoadDLL(StringView name)
  {
    return SDL_LoadObject(name.c_str());
  }

  SemaphoreHandle SDLOSSemaphoreCreate() 
  {
    const int i = sSDLSemaphoreIds.Alloc();
    SDL_sem* sem = SDL_CreateSemaphore( 0 );
    sSDLSemaphores[ i ] = sem;
    TAC_ASSERT( sem );
    return i;
  }

  void            SDLOSSemaphoreDecrementWait( SemaphoreHandle semaphoreHandle ) 
  {
    SDL_sem* semaphore = sSDLSemaphores[ ( int )semaphoreHandle ];
    SDL_SemPost( semaphore );
  }

  void            SDLOSSemaphoreIncrementPost( SemaphoreHandle semaphoreHandle ) 
  {
    SDL_sem* semaphore = sSDLSemaphores[ ( int )semaphoreHandle ];
    SDL_SemPost( semaphore );
  }

  void SDLAppInit( Errors& errors )
  {
    DesktopAppInit( SDLPlatformSpawnWindow,
                    SDLPlatformDespawnWindow,
                    SDLPlatformGetMouseHoveredWindow,
                    SDLPlatformFrameBegin,
                    SDLPlatformFrameEnd,
                    SDLPlatformWindowMoveControls,
                    SDLPlatformWindowResizeControls,
                    errors );

  }

  void SDLOSInit( Errors& errors )
  {
    TAC_RAISE_ERROR_IF( SDL_Init( SDL_INIT_EVERYTHING ), SDL_GetError(), errors );

    OS::OSDebugBreak = SDLOSDebugBreak;
    OS::OSDebugPopupBox = SDLOSDebugPopupBox;
    OS::OSGetApplicationDataPath = SDLOSGetApplicationDataPath;
    OS::OSGetFileLastModifiedTime = SDLOSGetFileLastModifiedTime;
    OS::OSGetFilesInDirectory = SDLOSGetFilesInDirectory;
    OS::OSGetDirectoriesInDirectory = SDLOSGetDirectoriesInDirectory;
    OS::OSSaveDialog = SDLOSSaveDialog;
    OS::OSOpenDialog = SDLOSOpenDialog;
    OS::OSGetPrimaryMonitor = SDLOSGetPrimaryMonitor;
    OS::OSSetScreenspaceCursorPos = SDLOSSetScreenspaceCursorPos;
    OS::OSGetLoadedDLL = SDLOSGetLoadedDLL;
    OS::OSLoadDLL = SDLOSLoadDLL;
    OS::OSSemaphoreCreate = SDLOSSemaphoreCreate;
    OS::OSSemaphoreDecrementWait = SDLOSSemaphoreDecrementWait;
    OS::OSSemaphoreIncrementPost = SDLOSSemaphoreIncrementPost;
  }

} // namespace Tac
