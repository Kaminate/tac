#pragma once
#include "common/tacRenderer.h"
// TODO: remove?
#include "GL/gl3w.h"

struct TacOpenGL4Renderer : public TacRenderer
{
  void Init( TacErrors& errors ) override;
  void CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors ) override;
};

struct TacOpenGL4Globals
{
  static TacOpenGL4Globals* Instance();

  void( *mInitOpenGlStuff )(
    TacOpenGL4Renderer* OpenGL4Renderer,
    struct TacDesktopWindow* desktopWindow,
    struct TacErrors& errors );
};
