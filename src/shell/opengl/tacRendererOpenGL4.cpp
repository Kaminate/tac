/*
#include "RendererOpenGL4.h"
#include "src/common/Shell.h"


const static int includedOpenGL4RendererFactory = []()
{
  static struct OpenGL4RendererFactory : public RendererFactory
  {
    OpenGL4RendererFactory()
    {
      mRendererName = RendererNameOpenGL4;
    }
    void CreateRenderer() override
    {
      new OpenGL4Renderer;
    }
  } OpenGL4Factory;
  RendererRegistry::Instance().mFactories.push_back( &OpenGL4Factory );
  return 0;
}( );




void OpenGL4Renderer::Init( Errors& errors )
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
    errors += ToString( major ).c_str();
    errors += " ";
    errors += ToString( minor ).c_str();
    errors += " unsupported";
    return;
  }


}

void OpenGL4Renderer::CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors )
{
  OpenGL4Globals* globals = OpenGL4Globals::Instance();
  globals->mInitOpenGlStuff( this, desktopWindow, errors );
}
OpenGL4Globals* OpenGL4Globals::Instance()
{
  static OpenGL4Globals vg;
  return &vg;
}
*/
