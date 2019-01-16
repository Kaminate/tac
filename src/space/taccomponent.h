#pragma once
#include "common/tacSerialization.h"
#include "tacspacetypes.h"
#include <map>

struct TacEntity;

struct TacComponentData
{
  const char* mName = nullptr;
  TacSystemType mSystemType = TacSystemType::Count;
  TacVector< TacNetworkBit > mNetworkBits;
};

struct TacComponent
{
  virtual ~TacComponent() = default;
  virtual void TacDebugImgui(){};
  virtual void PreReadDifferences(){};
  virtual void PostReadDifferences(){};
  virtual TacComponentType GetComponentType() = 0;
  TacEntity* mEntity = nullptr;
};

TacComponentData* TacGetComponentData( TacComponentType componentType );
char TacComponentToBitField( TacComponentType componentType );
