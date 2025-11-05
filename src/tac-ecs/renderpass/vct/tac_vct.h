#pragma once

// VCT = Voxel Cone Tracing
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  struct VCTVolume
  {
    v3 mCenter;
    v3 mExtents;
  };

  struct VCTApi
  {
    static void CreateVolume() {}
  };

}

// ----------

#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/system/tac_system.h"

namespace Tac
{
  struct VCTComponent : public Component
  {
    //virtual ~Component() = default;
    //virtual void PreReadDifferences() {};
    //virtual void PostReadDifferences() {};
    //virtual auto GetEntry() const -> const ComponentInfo* = 0;
    //void         CopyFrom( const Component* );

    //Entity* mEntity {};
  };

  struct VCTSystem : public System
  {
    void DebugImgui() override {};
    void Update() override     {};
  };
}

