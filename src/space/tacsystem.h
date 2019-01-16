#pragma once

#include "tacspacetypes.h"

struct TacWorld;
struct TacComponent;
struct TacSystem
{
  TacWorld* mWorld = nullptr;
  virtual TacComponent* CreateComponent( TacComponentType componentType ) = 0;
  virtual void DestroyComponent( TacComponent* component ) = 0;
  virtual void DebugImgui(){};
  virtual void Update(){};
  virtual TacSystemType GetSystemType() = 0;
};
