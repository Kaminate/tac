#include "tac_render_tutorial.h" // self-inc


#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"

auto Tac::RenderTutorialCreateWindow( StringView name, Errors& errors ) -> Tac::WindowHandle
{
  const Monitor monitor { OS::OSGetPrimaryMonitor() };
  const v2i windowSize{ monitor.mSize / 2 };
  const v2i windowPos{ ( monitor.mSize - windowSize ) / 2 };
  const WindowCreateParams windowCreateParams
  {
    .mName { name },
    .mPos  { windowPos },
    .mSize { windowSize },
  };
  TAC_CALL_RET( const WindowHandle windowHandle{
    AppWindowApi::CreateWindow( windowCreateParams, errors ) } );
  return windowHandle;
}

namespace Tac
{
  NDCSpacePosition3::NDCSpacePosition3( v3 v ) : mValue( v ) {}
  NDCSpacePosition3::NDCSpacePosition3( float x, float y, float z ) : mValue{ x, y, z } {}
  LinearColor3::LinearColor3( v3 v ) : mValue( v ) {}
  LinearColor3::LinearColor3( float x, float y, float z ) : mValue{ x, y, z } {}
  TextureCoordinate2::TextureCoordinate2( v2 v ) : mValue( v ) {}
  TextureCoordinate2::TextureCoordinate2( float x, float y ) : mValue{ x, y } {}
}
