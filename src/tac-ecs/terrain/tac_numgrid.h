#pragma once

#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac
{
  struct NumGrid : public Component
  {
    struct WorldspaceCorners { v3 mBL; v3 mTL; v3 mTR; v3 mBR; };

    static auto GetComponent( const Entity* ) -> const NumGrid*;
    static auto GetComponent( dynmc Entity* ) -> dynmc NumGrid*;
    auto GetEntry() const -> const ComponentInfo* override;
    auto GetWorldspaceCorners() const -> WorldspaceCorners;

    using GridImages = Vector< AssetPathString >;

    AssetPathString mAsset  {};
    GridImages      mImages {};
    int             mWidth  {};
    int             mHeight {};
    Vector< u8 >    mData   {};
  };

  struct NumGridSys : public System
  {
    struct RenderParams
    {
      Render::IContext*     mContext            {};
      const Camera*         mCamera             {};
      v2i                   mViewSize           {};
      Render::TextureHandle mColor              {};
      Render::TextureHandle mDepth              {};
    };

    void Update() override;
    void DebugImgui() override;
    void DebugRender( const RenderParams&, Errors& );

    static void SpaceInitNumGrid();
    static auto GetSystem( dynmc World* ) -> dynmc NumGridSys*;
    static auto GetSystem( const World* ) -> const NumGridSys*;

    template<typename T> void ForEachNumGrid( T&& t ) const
    {
      for( NumGrid* numGrid : mNumGrids )
        t( numGrid );
    }

    using NumGrids = Set< NumGrid* >;

    NumGrids mNumGrids {};
  };

} // namespace Tac

