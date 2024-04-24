#include "tac_desktop_window_life.h"

//#include "tac-std-lib/identifier/tac_id_collection.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-engine-core/system/tac_platform.h"
#include "tac-desktop-app/tac_desktop_app.h"

namespace Tac
{
  using WindowRequestsCreate = FixedVector< PlatformSpawnWindowParams, kDesktopWindowCapacity >;
  using WindowRequestsDestroy = FixedVector< DesktopWindowHandle, kDesktopWindowCapacity >;

  static int sIDCounter;
  static Vector<int> sFreeIds;
  //static IdCollection                  sDesktopWindowHandleIDs;
  static std::mutex                    sWindowHandleLock;
  static WindowRequestsCreate          sWindowRequestsCreate;
  static WindowRequestsDestroy         sWindowRequestsDestroy;

  static void                FreeDesktopWindowHandle( DesktopWindowHandle handle )
  {
    sFreeIds.push_back( handle.GetIndex() );
  }

  static DesktopWindowHandle AllocDesktopWindowHandle()
  {
      DesktopWindowHandle handle;
      if( sFreeIds.empty() )
      {
        handle = sIDCounter++;
      }
      else
      {
        handle = sFreeIds.back();
        sFreeIds.pop_back();
      }

      return handle;

  }
}

void Tac::DesktopAppUpdateWindowRequests( Errors& errors )
{
  sWindowHandleLock.lock();
  WindowRequestsCreate requestsCreate = sWindowRequestsCreate;
  WindowRequestsDestroy requestsDestroy = sWindowRequestsDestroy;
  sWindowRequestsCreate.clear();
  sWindowRequestsDestroy.clear();
  sWindowHandleLock.unlock();

  PlatformFns* platform = PlatformFns::GetInstance();

  for( const PlatformSpawnWindowParams& info : requestsCreate )
    platform->PlatformSpawnWindow( info, errors );

  for( const DesktopWindowHandle desktopWindowHandle : requestsDestroy )
    platform->PlatformDespawnWindow( desktopWindowHandle );
}


Tac::DesktopWindowHandle Tac::DesktopAppImplCreateWindow( const DesktopAppCreateWindowParams& desktopParams )
{
    sWindowHandleLock.lock();

    //const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    const DesktopWindowHandle handle = AllocDesktopWindowHandle();

    const PlatformSpawnWindowParams info =
    {
      .mHandle { handle },
      .mName { desktopParams }.mName,
      .mX { desktopParams.mX },
      .mY { desktopParams.mY },
      .mWidth { desktopParams.mWidth },
      .mHeight { desktopParams.mHeight },
    };
    sWindowRequestsCreate.push_back( info );
    sWindowHandleLock.unlock();
    return handle;
}


void                Tac::DesktopAppImplDestroyWindow( const DesktopWindowHandle& desktopWindowHandle )
{
  if( !desktopWindowHandle.IsValid() )
    return;

  sWindowHandleLock.lock();
  //sDesktopWindowHandleIDs.Free( ( int )desktopWindowHandle );
  FreeDesktopWindowHandle( desktopWindowHandle );
  sWindowRequestsDestroy.push_back( desktopWindowHandle );
  sWindowHandleLock.unlock();
}

