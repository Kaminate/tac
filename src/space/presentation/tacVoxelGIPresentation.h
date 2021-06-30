#pragma once

namespace Tac
{
  struct World;
  struct Camera;
  struct Errors;
  void               VoxelGIPresentationInit( Errors& );
  void               VoxelGIPresentationUninit();
  void               VoxelGIPresentationRender( const World*,
                                                const Camera*,
                                                int viewWidth,
                                                int viewHeight,
                                                Render::ViewHandle );
  //bool&              VoxelGIPresentationGetEnabled();
  //bool&              VoxelGIPresentationGetDebugEnabled();
  void               VoxelGIDebugImgui();
}

