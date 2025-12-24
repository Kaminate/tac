#pragma once

#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct NumGrid : public Component
  {
    static auto GetComponent( Entity* ) -> NumGrid*;
    auto GetEntry() const -> const ComponentInfo* override;

    AssetPathString mAsset  {};
    int             mWidth  { 32 };
    int             mHeight { 32 };
    Vector<u8>      mData   {};
  };

  struct NumGridSys : public System
  {
    auto CreateNumGrid() -> NumGrid*;
    void DestroyNumGrid( NumGrid* );

    void Update() override;
    void DebugImgui() override;

    static void SpaceInitNumGrid();
    static auto GetSystem( dynmc World* ) -> dynmc NumGridSys*;
    static auto GetSystem( const World* ) -> const NumGridSys*;

    template<typename T> void ForEachNumGrid( T&& t ) const
    {
      for( NumGrid* numGrid : mNumGrids )
        t( numGrid );
    }

    static SystemInfo* sInfo;

    using NumGrids = Set< NumGrid* >;
    NumGrids mNumGrids {};
  };

} // namespace Tac

