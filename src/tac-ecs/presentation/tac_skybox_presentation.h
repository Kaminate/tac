#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-rhi/render3/tac_render_api.h"

#define TAC_SKYBOX_PRESENTATION_ENABLED() 1

#if TAC_SKYBOX_PRESENTATION_ENABLED()

namespace Tac
{
  struct SkyboxRenderParams
  {
    const Camera*         mCamera;
    v2i                   mViewSize;
    Render::TextureHandle mViewId;
    AssetPathStringView   mSkyboxDir;
  };

  void SkyboxPresentationInit( Errors& );
  void SkyboxPresentationUninit();
  void SkyboxPresentationRender( SkyboxRenderParams, Errors& );

}

#endif
