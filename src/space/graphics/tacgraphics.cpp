#include "src/common/tacPreprocessor.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacMemory.h"
#include "src/space/tacWorld.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/tacComponent.h"
#include <cmath>
namespace Tac
{

//static const Vector< ComponentRegistryEntryIndex > managedComponentTypes = {
//  ComponentRegistryEntryIndex::Model,
//};

Model* Graphics::CreateModelComponent()
{
  auto model = TAC_NEW Model;
  mModels.insert( model );
  return model;
}


Graphics* Graphics::GetSystem( World* world )
{
  return ( Graphics* )world->GetSystem( Graphics::GraphicsSystemRegistryEntry );
}

//Component* Graphics::CreateComponent( ComponentRegistryEntryIndex componentType )
//{
//  switch( componentType )
//  {
//    //case ComponentRegistryEntryIndex::Say:
//    //{
//    //  auto say = TAC_NEW Say;
//    //  mSays.insert( say );
//    //  return say;
//    //}
//
//  case ComponentRegistryEntryIndex::Model:
//  {
//    auto model = TAC_NEW Model;
//    mModels.insert( model );
//    return model;
//  }
//  }
//  InvalidCodePath;
//  return nullptr;
//}

void Graphics::DestroyModelComponent( Model*model )
{
  auto it = mModels.find( model );
  TAC_ASSERT( it != mModels.end() );
  mModels.erase( it );
  delete model;
}
//void Graphics::DestroyComponent( Component* component )
//{
//  auto componentType = component->GetComponentType();
//  switch( componentType )
//  {
//    //  case ComponentRegistryEntryIndex::Say:
//    //  {
//    //    auto say = ( Say* )component;
//    //    auto it = mSays.find( say );
//    //    Assert( it != mSays.end() );
//    //    mSays.erase( it );
//    //    delete say;
//    //  } return;
//
//  case ComponentRegistryEntryIndex::Model:
//  {
//    auto model = ( Model* )component;
//    auto it = mModels.find( model );
//    Assert( it != mModels.end() );
//    mModels.erase( it );
//    delete model;
//  }return;
//  }
//  InvalidCodePath;
//}

//const Vector< ComponentRegistryEntryIndex >& Graphics::GetManagedComponentTypes()
//{
//  return managedComponentTypes;
//}
void Graphics::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Graphics" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );

  ImGuiText("graphics::debugimgiuo()");
}

SystemRegistryEntry* Graphics::GraphicsSystemRegistryEntry;
static System* CreateGraphicsSystem()
{
  return TAC_NEW Graphics;
}

void GraphicsDebugImgui(System*);
void Graphics::SpaceInitGraphics()
{
  Graphics::GraphicsSystemRegistryEntry = SystemRegistry::Instance()->RegisterNewEntry();
  Graphics::GraphicsSystemRegistryEntry->mCreateFn = CreateGraphicsSystem;
  Graphics::GraphicsSystemRegistryEntry->mName = "Graphics";
  Graphics::GraphicsSystemRegistryEntry->mDebugImGui = GraphicsDebugImgui;
  Model::SpaceInitGraphicsModel();
}


}

