#pragma once

#include "tac-rhi/renderer/tac_renderer.h"


namespace Tac
{
  // Takes as input a depth texture, which contains non-linear values from [0,1]
  // and returns a linearized greyscale texture
  //
  // w - desired width of output texture 
  // h - desired height of output texture 
  // f - far plane camera dist
  // n - near plane camera dist
  Render::TextureHandle DepthBufferLinearVisualizationRender( Render::TextureHandle,
                                                              int w,
                                                              int h,
                                                              Render::InProj );

}
