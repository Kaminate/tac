#pragma once
#include "tacspacetypes.h"
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h"
#include "common/math/tacVector3.h"
#include "common/math/tacMatrix4.h"
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
  void Unparent();
  void AddChild( TacEntity* child );

  TacEntity* mParent = nullptr;
  TacVector< TacEntity* > mChildren;
  TacWorld* mWorld = nullptr;
  TacEntityUUID mEntityUUID = TacNullEntityUUID;
  std::list< TacComponent* > mComponents;

  v3 mLocalPosition = {};
  v3 mLocalEulerRads = {};
  v3 mLocalScale = { 1, 1, 1 };

  m4 mLocalTransform;
  m4 mWorldTransform;
  m4 mWorldTransformNoScale;

  TacString mName;
};

const TacVector< TacNetworkBit > TacEntityBits =
{
  { "mPosition", TacOffsetOf( TacEntity, mLocalPosition ), sizeof( float ), 3 },
};

