#include "space/model/tacmodel.h"
#include "space/tacentity.h"
#include "space/graphics/tacgraphics.h"
#include "common/graphics/imgui/tacImGui.h"
#include "common/tacOS.h"
#include "common/tacUtility.h"
#include "common/tacErrorHandling.h"
#include "common/tacJson.h"

//#include "common\tacAssetManager.h"
//#include "common\tacPlatform.h"
//#include "core\tacstuff.h"
//#include "core\tacentity.h"
//#include "core\tacworld.h"
//#include <iostream>
//#include <fstream>


TacComponentRegistryEntry* TacModel::ModelComponentRegistryEntry;

const TacModel* TacModel::GetModel( const TacEntity* entity )
{
  return ( TacModel* )entity->GetComponent( TacModel::ModelComponentRegistryEntry );
}
TacModel* TacModel::GetModel( TacEntity* entity )
{
  return ( TacModel* )entity->GetComponent( TacModel::ModelComponentRegistryEntry );
}
TacComponentRegistryEntry* TacModel::GetEntry()
{
  return TacModel::ModelComponentRegistryEntry;
}


static TacComponent* TacCreateModelComponent( TacWorld* world )
{
  return TacGraphics::GetSystem( world )->CreateModelComponent();
}

static void TacDestroyModelComponent( TacWorld* world, TacComponent* component )
{
  TacGraphics::GetSystem( world )->DestroyModelComponent( ( TacModel* )component );
}

static void TacSaveModelComponent( TacJson& modelJson, TacComponent* component )
{
  auto model = ( TacModel* )component;
  TacJson colorRGBJson;
  colorRGBJson[ "r" ] = model->mColorRGB[ 0 ];
  colorRGBJson[ "g" ] = model->mColorRGB[ 1 ];
  colorRGBJson[ "b" ] = model->mColorRGB[ 2 ];

  modelJson[ "mGLTFPath" ] = model->mGLTFPath;
  modelJson[ "mColorRGB" ] = colorRGBJson;
}

static void TacLoadModelComponent( TacJson& modelJson, TacComponent* component )
{
  auto model = ( TacModel* )component;
  model->mGLTFPath = ( modelJson )[ "mGLTFPath" ].mString;
  model->mColorRGB = {
    ( float )( modelJson )[ "mColorRGB" ][ "r" ].mNumber,
    ( float )( modelJson )[ "mColorRGB" ][ "g" ].mNumber,
    ( float )( modelJson )[ "mColorRGB" ][ "b" ].mNumber };
}


  void TacModelDebugImgui( TacComponent* );
void TacModel::TacSpaceInitGraphicsModel()
{
  TacModel::ModelComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacModel::ModelComponentRegistryEntry->mName = "Model";
  TacModel::ModelComponentRegistryEntry->mNetworkBits = TacComponentModelBits;
  TacModel::ModelComponentRegistryEntry->mCreateFn = TacCreateModelComponent;
  TacModel::ModelComponentRegistryEntry->mDestroyFn = TacDestroyModelComponent;
  TacModel::ModelComponentRegistryEntry->mDebugImguiFn = TacModelDebugImgui;
  TacModel::ModelComponentRegistryEntry->mSaveFn = TacSaveModelComponent;
  TacModel::ModelComponentRegistryEntry->mLoadFn = TacLoadModelComponent;
}
