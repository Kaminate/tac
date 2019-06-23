#include "common/tacPreprocessor.h"
#include "common/graphics/tacFont.h"
#include "common/math/tacMath.h"
#include "space/tacworld.h"
#include "space/tacgraphics.h"
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

TacSystemRegistryEntry* TacGraphics::SystemRegistryEntry = []()
{
  TacSystemRegistryEntry* entry = TacSystemRegistry::Instance()->RegisterNewEntry();
  entry->mCreateFn = []() -> TacSystem* { return new TacGraphics; };
  return entry;
}( );

TacGraphics* TacGraphics::GetSystem( TacWorld* world )
{
  return ( TacGraphics* )world->GetSystem( TacGraphics::SystemRegistryEntry );
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
}
