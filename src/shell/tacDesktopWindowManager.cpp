#include "src/shell/tacDesktopWindowManager.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/containers/tacFixedVector.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/tacOS.h"
#include "src/common/tacKeyboardinput.h" // temp
#include <thread>

namespace Tac
{

  struct WantSpawnInfo
  {
    DesktopWindowHandle mHandle;
    int mX;
    int mY;
    int mWidth;
    int mHeight;
  };


  //struct
  //{
  //  void Init()
  //  {
  //    sDesktopWindowHandleIDs.Init( kMaxDesktopWindowStateCount );
  //  }

  //  IdCollection* GetDesktopWindowHandleIDs( BathroomKey* bathroomKey )
  //  {
  //    sWindowHandleLock.lock();
  //    bathroomKey->mCallback = []() { sBathroom.sWindowHandleLock.unlock(); };
  //    return &sDesktopWindowHandleIDs;
  //  }

  std::mutex sWindowHandleLock;
  IdCollection sDesktopWindowHandleIDs;
  //} sBathroom;

  typedef FixedVector< WantSpawnInfo, kMaxDesktopWindowStateCount > WindowRequests;
  static WindowRequests sWindowRequests;
  //static DesktopWindow* mDesktopWindows[ kMaxDesktopWindowStateCount ] = {};

  WindowHandleIterator::WindowHandleIterator()
  {
    sWindowHandleLock.lock();
  }

  WindowHandleIterator::~WindowHandleIterator()
  {
    sWindowHandleLock.unlock();

  }
  int* WindowHandleIterator::begin()
  {
    return sDesktopWindowHandleIDs.begin();
  }
  int* WindowHandleIterator::end()
  {
    return sDesktopWindowHandleIDs.end();
  }

//  WindowHandleIterator GetDesktopWindowHandleIDs()
//  {
//WindowHandleIterator
//  }

  void DesktopWindowManagerInit()
  {
    sDesktopWindowHandleIDs.Init( kMaxDesktopWindowStateCount );
  }

  //DesktopWindowHandle* GetDesktopWindowHandles()
  //{
  //  sWindowHandleLock.lock();
  //  DesktopWindowHandle* result = FrameMemory::Allocate(
  //    sizeof( DesktopWindowHandle ) *
  //    sDesktopWindowHandleIDs. );

  //    void* Allocate( int );
  //  sWindowHandleLock.unlock();
  //}


  //void DesktopWindowManager::SetWindowParams( WindowParams windowParams )
  //{
  //  WindowParams* pParams = FindWindowParams( windowParams.mName );
  //  if( pParams )
  //    *pParams = windowParams;
  //  else
  //    mWindowParams.push_back( windowParams );
  //}

  //WindowParams* DesktopWindowManager::FindWindowParams( StringView windowName )
  //{
  //  for( WindowParams& data : mWindowParams )
  //    if( data.mName == windowName )
  //      return &data;
  //  return nullptr;
  //}

  //void DesktopWindowManager::DoWindow( StringView windowName )
  //{
  //  DesktopWindow* window = DesktopApp::Instance->FindWindow( windowName );
  //  if( !window )
  //    mWantSpawnWindows.push_back( windowName );
  //}

  void DesktopWindowPollCreationRequests( Errors& errors )
  {
    WindowRequests spawnRequests;
    sWindowHandleLock.lock();
    spawnRequests = sWindowRequests;
    sWindowRequests.clear();
    sWindowHandleLock.unlock();
    for( WantSpawnInfo info : spawnRequests )
    {
      //DesktopApp::Instance->SpawnWindow( info.mHandle,
      //                                   info.mX,
      //                                   info.mY,
      //                                   info.mWidth,
      //                                   info.mHeight );
      TAC_HANDLE_ERROR( errors );
    }
  }

  static bool usedWindowHandles[ kMaxDesktopWindowStateCount ];

  DesktopWindowHandle DesktopWindowCreate( int x, int y, int width, int height )
  {
    static int gWindowCounter;
    std::lock_guard< std::mutex > lock( sWindowHandleLock );
    DesktopWindowHandle handle = { gWindowCounter++ };
    WantSpawnInfo info;
    info.mX = x;
    info.mY = y;
    info.mWidth = width;
    info.mHeight = height;
    info.mHandle = handle;
    sWindowRequests.push_back( info );
    return handle;
  }

  //DesktopWindow* DesktopApp::FindDesktopWindow( DesktopWindowHandle desktopWindowHandle )
  //{
  //  for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
  //  {
  //    DesktopWindow* desktopWindow = mDesktopWindows[ i ];
  //    if( !desktopWindow )
  //      continue;
  //    if( AreWindowHandlesEqual( desktopWindow->mHandle, desktopWindowHandle ) )
  //      return desktopWindow;
  //  }
  //  return nullptr;
  //}

  //void DesktopApp::SpawnWindow( DesktopWindow* desktopWindow )
  //{
  //  for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
  //  {
  //    if( mDesktopWindows[ i ] )
  //      continue;
  //    mDesktopWindows[ i ] = desktopWindow;
  //    return;
  //  }
  //}
}
