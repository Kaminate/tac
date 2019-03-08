#include "space/taccomponent.h"
//#include "space/tacsay.h"
#include "space/tacmodel.h"
#include "space/taccollider.h"
//#include "space/tacterrain.h"

char TacComponentToBitField( TacComponentType componentType )
{
  char result = 1 << ( char )componentType;
  return result;
}

TacComponentData* TacGetComponentData( TacComponentType componentType )
{
  /////////////
  // BE      //
  //         //
  // VERY    //
  //         //
  // CAREFUL //
  //         //
  // OF      //
  //         //
  // COPY    //
  //         //
  // PASTA   //
  /////////////

  // Ahh who am i kidding you're going to fuck it up anyway


  static TacComponentData* mRegisteredComponents[ ( int )TacComponentType::Count ];
  static bool initialized;
  if( !initialized )
  {
    initialized = true;

    auto colliderData = new TacComponentData();
    colliderData->mName = "Collider";
    colliderData->mSystemType = TacSystemType::Physics;
    colliderData->mNetworkBits = TacColliderBits;
    mRegisteredComponents[ ( int ) TacComponentType::Collider ] = colliderData;

  }
  {
    //{
    //  TacComponentType::Say,
    //  new TacComponentData(
    //    "Say",
    //    TacSystemType::Graphics,
    //    TacSayBits)
    //},
    {
      auto modelData = new TacComponentData;
      modelData->mName = "Model";
      modelData->mSystemType = TacSystemType::Graphics;
      modelData->mNetworkBits = TacComponentModelBits;
      mRegisteredComponents[ ( int )TacComponentType::Model ] = modelData;
    }
    /*
    {
      TacComponentType::Terrain,
      new TacComponentData(
        "Terrain",
        TacSystemType::Physics,
        TacTerrainBits )
    },
    */
  };
  return mRegisteredComponents[ ( int )componentType ];
}

