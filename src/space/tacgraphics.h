#pragma once
#include "common/graphics/tacRenderer.h"
#include "common/tacLocalization.h"
#include "common/containers/tacVector.h"
#include "space/model/tacmodel.h"
#include "space/tacsystem.h"
//#include "graphics/tacDefaultGeometry.h"
#include <set>

struct TacFontStuff;
//struct TacSay;
//struct TacModel;
//struct TacComponent;


struct TacGraphics : public TacSystem
{
  //const TacVector< TacComponentRegistryEntryIndex >& GetManagedComponentTypes() override;
  //TacComponent* CreateComponent( TacComponentRegistryEntryIndex componentType ) override;
  //void DestroyComponent( TacComponent* component ) override;
  //TacSystemType GetSystemType() override { return TacSystemType::Graphics; }

  static TacGraphics* GetSystem( TacWorld* world );
  static TacSystemRegistryEntry* SystemRegistryEntry;
  //TacSystemRegistryEntry* GetEntry() override;

  TacModel* CreateModelComponent();
  void DestroyModelComponent(TacModel* );

  void DebugImgui() override;

  // Note about DebugDraw...() prototypes:
  //
  // When drawing a shape that contains multiple points, we should be able to specify
  // different colors for each point
  //
  // For convenience, we also offer an overload that defaults all points to white


  //std::set< TacSay* > mSays;
  //std::set< TacStuff* > mStuffs;
  std::set< TacModel* > mModels;
};
