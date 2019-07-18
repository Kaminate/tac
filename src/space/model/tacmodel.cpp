#include "space/model/tacmodel.h"
#include "space/tacentity.h"
#include "space/graphics/tacgraphics.h"
#include "common/graphics/tacImGui.h"
#include "common/tacOS.h"
#include "common/tacUtility.h"
#include "common/tacErrorHandling.h"

//#include "common\tacAssetManager.h"
//#include "common\tacPlatform.h"
//#include "core\tacstuff.h"
//#include "core\tacentity.h"
//#include "core\tacworld.h"
//#include <iostream>
//#include <fstream>



const TacModel* TacModel::GetModel( const TacEntity* entity )
{
  return ( TacModel* )entity->GetComponent( TacModel::ComponentRegistryEntry );
}
TacModel* TacModel::GetModel( TacEntity* entity )
{
  return ( TacModel* )entity->GetComponent( TacModel::ComponentRegistryEntry );
}
TacComponentRegistryEntry* TacModel::GetEntry()
{
  return TacModel::ComponentRegistryEntry;
}

