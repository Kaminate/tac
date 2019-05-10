#pragma once
#include "tacspacetypes.h"
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h"
#include "common/math/tacVector3.h"
#include <list>

struct TacWorld;
struct TacComponent;

struct TacEntity
{
  ~TacEntity();
  void RemoveAllComponents();
  TacComponent* GetComponent( TacComponentType type );
  const TacComponent* GetComponent( TacComponentType type ) const;
  bool HasComponent( TacComponentType componentType );
  TacComponent* AddNewComponent( TacComponentType componentType );
  void RemoveComponent( TacComponentType type );
  void DeepCopy( const TacEntity& );
  void TacDebugImgui();
  //void TacIntegrate( float time );

  TacWorld* mWorld = nullptr;
  TacEntityUUID mEntityUUID = TacNullEntityUUID;
  std::list< TacComponent* > mComponents;
  v3 mPosition = {};
  v3 mEulerRads = {};
  v3 mScale = { 1, 1, 1 };
  TacString mName;
};

const TacVector< TacNetworkBit > TacEntityBits =
{
  { "mPosition", TacOffsetOf( TacEntity, mPosition ), sizeof( float ), 3 },
};

