#include "common/tacPreprocessor.h"
#include "common/graphics/tacFont.h"
#include "common/graphics/imgui/tacImGui.h"
#include "common/math/tacMath.h"
#include "space/tacworld.h"
#include "space/graphics/tacgraphics.h"
#include "space/taccomponent.h"

//#include "tacsay.h"
//#include "tacmodel.h"

#include <cmath>

//static const TacVector< TacComponentRegistryEntryIndex > managedComponentTypes = {
//  TacComponentRegistryEntryIndex::Model,
//};

TacModel* TacGraphics::CreateModelComponent()
{
  auto model = new TacModel();
  mModels.insert( model );
  return model;
}


TacGraphics* TacGraphics::GetSystem( TacWorld* world )
{
  return ( TacGraphics* )world->GetSystem( TacGraphics::GraphicsSystemRegistryEntry );
}

//TacComponent* TacGraphics::CreateComponent( TacComponentRegistryEntryIndex componentType )
//{
//  switch( componentType )
//  {
//    //case TacComponentRegistryEntryIndex::Say:
//    //{
//    //  auto say = new TacSay();
//    //  mSays.insert( say );
//    //  return say;
//    //}
//
//  case TacComponentRegistryEntryIndex::Model:
//  {
//    auto model = new TacModel();
//    mModels.insert( model );
//    return model;
//  }
//  }
//  TacInvalidCodePath;
//  return nullptr;
//}

void TacGraphics::DestroyModelComponent( TacModel*model )
{
  auto it = mModels.find( model );
  TacAssert( it != mModels.end() );
  mModels.erase( it );
  delete model;
}
//void TacGraphics::DestroyComponent( TacComponent* component )
//{
//  auto componentType = component->GetComponentType();
//  switch( componentType )
//  {
//    //  case TacComponentRegistryEntryIndex::Say:
//    //  {
//    //    auto say = ( TacSay* )component;
//    //    auto it = mSays.find( say );
//    //    TacAssert( it != mSays.end() );
//    //    mSays.erase( it );
//    //    delete say;
//    //  } return;
//
//  case TacComponentRegistryEntryIndex::Model:
//  {
//    auto model = ( TacModel* )component;
//    auto it = mModels.find( model );
//    TacAssert( it != mModels.end() );
//    mModels.erase( it );
//    delete model;
//  }return;
//  }
//  TacInvalidCodePath;
//}

//const TacVector< TacComponentRegistryEntryIndex >& TacGraphics::GetManagedComponentTypes()
//{
//  return managedComponentTypes;
//}
void TacGraphics::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Graphics" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );

  TacImGuiText("graphics::debugimgiuo()");
}

TacSystemRegistryEntry* TacGraphics::GraphicsSystemRegistryEntry;
static TacSystem* TacCreateGraphicsSystem()
{
  return new TacGraphics;
}

void TacGraphicsDebugImgui(TacSystem*);
void TacGraphics::TacSpaceInitGraphics()
{
  TacGraphics::GraphicsSystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacGraphics::GraphicsSystemRegistryEntry->mCreateFn = TacCreateGraphicsSystem;
  TacGraphics::GraphicsSystemRegistryEntry->mName = "Graphics";
  TacGraphics::GraphicsSystemRegistryEntry->mDebugImGui = TacGraphicsDebugImgui;
  TacModel::TacSpaceInitGraphicsModel();
}

