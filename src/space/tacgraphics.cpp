#include "common/tacPreprocessor.h"
#include "common/graphics/tacFont.h"
#include "common/math/tacMath.h"
#include "space/tacgraphics.h"
#include "space/taccomponent.h"

//#include "tacsay.h"
//#include "tacmodel.h"

#include <cmath>

static const TacVector< TacComponentType > managedComponentTypes = {
  TacComponentType::Model,
};


TacComponent* TacGraphics::CreateComponent( TacComponentType componentType )
{
  switch( componentType )
  {
    //case TacComponentType::Say:
    //{
    //  auto say = new TacSay();
    //  mSays.insert( say );
    //  return say;
    //}

  case TacComponentType::Model:
  {
    auto model = new TacModel();
    mModels.insert( model );
    return model;
  }
  }
  TacInvalidCodePath;
  return nullptr;
}

void TacGraphics::DestroyComponent( TacComponent* component )
{
  auto componentType = component->GetComponentType();
  switch( componentType )
  {
    //  case TacComponentType::Say:
    //  {
    //    auto say = ( TacSay* )component;
    //    auto it = mSays.find( say );
    //    TacAssert( it != mSays.end() );
    //    mSays.erase( it );
    //    delete say;
    //  } return;

  case TacComponentType::Model:
  {
    auto model = ( TacModel* )component;
    auto it = mModels.find( model );
    TacAssert( it != mModels.end() );
    mModels.erase( it );
    delete model;
  }return;
  }
  TacInvalidCodePath;
}

const TacVector< TacComponentType >& TacGraphics::GetManagedComponentTypes()
{
  return managedComponentTypes;
}
void TacGraphics::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Graphics" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );
}
