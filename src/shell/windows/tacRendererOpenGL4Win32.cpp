
#include "shell/opengl/tacRendererOpenGL4.h"
#include "shell/windows/tacWindows.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"

struct TacOpenGL4RendererWindowContext : public TacRendererWindowData
{
  HGLRC hglrc = {};
};


void InitOpenGL4Stuff(
  TacOpenGL4Renderer* OpenGL4Renderer,
  struct TacDesktopWindow* desktopWindow,
  struct TacErrors& errors )
{
  auto hWnd = ( HWND )desktopWindow->mOperatingSystemHandle;
  HDC hdc = GetDC( hWnd );
  DWORD flags =
    PFD_DRAW_TO_WINDOW |
    PFD_SUPPORT_OPENGL |
    PFD_DOUBLEBUFFER;
  PIXELFORMATDESCRIPTOR format = {};
  format.nSize = sizeof( PIXELFORMATDESCRIPTOR ); //WORD  
  format.nVersion = 1; //WORD  
  format.dwFlags = flags; //DWORD 
  format.iPixelType = PFD_TYPE_RGBA; //BYTE  
  format.cColorBits = 24; //BYTE  
  format.cRedBits = 0; //BYTE  
  format.cRedShift = 0; //BYTE  
  format.cGreenBits = 0; //BYTE  
  format.cGreenShift = 0; //BYTE  
  format.cBlueBits = 0; //BYTE  
  format.cBlueShift = 0; //BYTE  
  format.cAlphaBits = 0; //BYTE  
  format.cAlphaShift = 0; //BYTE  
  format.cAccumBits = 0; //BYTE  
  format.cAccumRedBits = 0; //BYTE  
  format.cAccumGreenBits = 0; //BYTE  
  format.cAccumBlueBits = 0; //BYTE  
  format.cAccumAlphaBits = 0; //BYTE  
  format.cDepthBits = 32; //BYTE  
  format.cStencilBits = 0; //BYTE  
  format.cAuxBuffers = 0; //BYTE  
  format.iLayerType = PFD_MAIN_PLANE; //BYTE  
  format.bReserved = 0; //BYTE  
  format.dwLayerMask = 0; //DWORD 
  format.dwVisibleMask = 0; //DWORD 
  format.dwDamageMask = 0; //DWORD 

  // This should be called before wglcreatecontext
  BOOL succeeded = SetPixelFormat( hdc, 0, &format );
  if( succeeded == FALSE )
  {
    errors += TacGetLastWin32ErrorString().c_str();
    return;
  }

  // Create a new opengl rendering context suitable for drawing on the device referenced by the hdc.
  // The context has the same pixel format as the device context
  // this should be called after setpixelformat
  HGLRC hglrc = wglCreateContext( hdc );
  if( hglrc == NULL )
  {
    errors += TacGetLastWin32ErrorString().c_str();
    return;
  }

  wglMakeCurrent( hdc, hglrc );

  auto windowcontext = new TacOpenGL4RendererWindowContext();
  windowcontext->hglrc = hglrc;
  desktopWindow->mRendererData = windowcontext;
}

static int opengl4win32stuff = []()
{
  TacOpenGL4Globals* globals = TacOpenGL4Globals::Instance();
  globals->mInitOpenGlStuff = InitOpenGL4Stuff;


  return 0;
}( );

