#pragma once


namespace Tac
{
  struct DesktopWindowHandle;
  struct DesktopAppCreateWindowParams;
  struct Errors;
  void                DesktopAppUpdateWindowRequests( Errors& );
  DesktopWindowHandle DesktopAppImplCreateWindow( const DesktopAppCreateWindowParams& );
  void                DesktopAppImplDestroyWindow( const DesktopWindowHandle& );
}
