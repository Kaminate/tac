#pragma once

namespace Tac::Render { struct ViewHandle; }
namespace Tac { struct Errors; struct Camera; struct AssetPathStringView; }
namespace Tac
{
  void SkyboxPresentationInit( Errors& );
  void SkyboxPresentationUninit();
  void SkyboxPresentationRender( const Camera*,
                                 int viewWidth,
                                 int viewHeight,
                                 Render::ViewHandle viewId,
                                 const AssetPathStringView& skyboxDir );

}
