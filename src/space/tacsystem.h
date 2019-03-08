#pragma once

#include "common/containers/tacVector.h"
#include "space/tacspacetypes.h"

struct TacWorld;
struct TacComponent;
struct TacSystem
{
  virtual const TacVector< TacComponentType >& GetManagedComponentTypes() = 0;
  virtual TacComponent* CreateComponent( TacComponentType componentType ) = 0;
  virtual void DestroyComponent( TacComponent* component ) = 0;
  virtual void DebugImgui(){};
  virtual void Update(){};
  virtual TacSystemType GetSystemType() = 0;

  TacWorld* mWorld = nullptr;
};
