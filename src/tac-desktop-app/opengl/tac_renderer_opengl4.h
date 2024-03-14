/*
#pragma once
#include "src/common/graphics/Renderer.h"
#include "GL/gl3w.h"

struct OpenGL4Renderer : public Renderer
{
  void Init( Errors& errors ) override;
  void CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors ) override;
};

struct OpenGL4Globals
{
  static OpenGL4Globals* Instance();

  void( *mInitOpenGlStuff )(
    OpenGL4Renderer* OpenGL4Renderer,
    struct DesktopWindow* desktopWindow,
    struct Errors& errors );
};

*/
