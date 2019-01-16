#include "tacRendererOpenGL4.h"
#include "common/tacShell.h"


const static int includedOpenGL4RendererFactory = []()
{
  static struct TacOpenGL4RendererFactory : public TacRendererFactory
  {
    TacOpenGL4RendererFactory()
    {
      mRendererName = RendererNameOpenGL4;
    }
    void CreateRenderer( TacRenderer** renderer ) override
    {
      *renderer = new TacOpenGL4Renderer();
    }
  } OpenGL4Factory;
  TacRendererFactory::GetRegistry().push_back( &OpenGL4Factory );
  return 0;
}( );




void TacOpenGL4Renderer::Init( TacErrors& errors )
{
  // prequisite: need to create a platform-specific opengl context

  // https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)

  if( gl3wInit() )
  {
    errors = "Failed to init opengl";
    return;
  }

  int major = 4;
  int minor = 4;
  if( !gl3wIsSupported( major, minor ) )
  {
    errors += "opengl version";
    errors += TacToString( major ).c_str();
    errors += " ";
    errors += TacToString( minor ).c_str();
    errors += " unsupported";
    return;
  }


}

void TacOpenGL4Renderer::CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors )
{
  TacOpenGL4Globals* globals = TacOpenGL4Globals::Instance();
  globals->mInitOpenGlStuff( this, desktopWindow, errors );
}
TacOpenGL4Globals* TacOpenGL4Globals::Instance()
{
  static TacOpenGL4Globals vg;
  return &vg;
}
