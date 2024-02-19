#include "tac_desktop_window_life.h"

#include "src/common/identifier/tac_id_collection.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/containers/tac_fixed_vector.h"
#include "src/shell/tac_platform.h"
#include "src/shell/tac_desktop_app.h"

namespace Tac
{
  using WindowRequestsCreate = FixedVector< PlatformSpawnWindowParams, kDesktopWindowCapacity >;
  using WindowRequestsDestroy = FixedVector< DesktopWindowHandle, kDesktopWindowCapacity >;

  static IdCollection                  sDesktopWindowHandleIDs( kDesktopWindowCapacity );
  static std::mutex                    sWindowHandleLock;
  static WindowRequestsCreate          sWindowRequestsCreate;
  static WindowRequestsDestroy         sWindowRequestsDestroy;
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
    const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    const PlatformSpawnWindowParams info =
    {
      .mHandle = handle,
      .mName = desktopParams.mName,
      .mX = desktopParams.mX,
      .mY = desktopParams.mY,
      .mWidth = desktopParams.mWidth,
      .mHeight = desktopParams.mHeight,
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
  sDesktopWindowHandleIDs.Free( ( int )desktopWindowHandle );
  sWindowRequestsDestroy.push_back( desktopWindowHandle );
  sWindowHandleLock.unlock();
}

