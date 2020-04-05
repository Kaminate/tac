
#pragma once
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacLocalization.h"
#include "src/common/containers/tacVector.h"
#include "src/space/model/tacModel.h"
#include "src/space/tacSystem.h"
#include <set>


namespace Tac
{
struct FontStuff;


struct Graphics : public System
{
  //const Vector< ComponentRegistryEntryIndex >& GetManagedComponentTypes() override;
  //Component* CreateComponent( ComponentRegistryEntryIndex componentType ) override;
  //void DestroyComponent( Component* component ) override;
  //SystemType GetSystemType() override { return SystemType::Graphics; }

  static void SpaceInitGraphics();

  static Graphics* GetSystem( World* world );
  static SystemRegistryEntry* GraphicsSystemRegistryEntry;
  //SystemRegistryEntry* GetEntry() override;

  Model* CreateModelComponent();
  void DestroyModelComponent(Model* );

  void DebugImgui() override;

  // Note about DebugDraw...() prototypes:
  //
  // When drawing a shape that contains multiple points, we should be able to specify
  // different colors for each point
  //
  // For convenience, we also offer an overload that defaults all points to white


  //std::set< Say* > mSays;
  //std::set< Stuff* > mStuffs;
  std::set< Model* > mModels;
};

}

