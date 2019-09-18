#pragma once
#include "tacspacetypes.h"
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h"
#include "common/math/tacVector3.h"
#include "common/math/tacMatrix4.h"
#include <list>

struct TacWorld;
struct TacComponent;
struct TacComponentRegistryEntry;
struct TacJson;

struct TacRelativeSpace
{
  v3 mPosition = {};
  v3 mEulerRads = {};
  v3 mScale = { 1, 1, 1 };
};

struct TacEntity
{
  ~TacEntity();

  void RemoveAllComponents();
  
  TacComponent* GetComponent( TacComponentRegistryEntry* );
  const TacComponent* GetComponent( TacComponentRegistryEntry* ) const;
  bool HasComponent( TacComponentRegistryEntry* );
  TacComponent* AddNewComponent( TacComponentRegistryEntry* );
  void RemoveComponent( TacComponentRegistryEntry* );

  void DeepCopy( const TacEntity& );
  void TacDebugImgui();
  //void TacIntegrate( float time );
  void Unparent();
  void AddChild( TacEntity* child );
  void Save( TacJson& entityJson );
  void Load( TacJson& entityJson );

  TacEntity* mParent = nullptr;
  TacVector< TacEntity* > mChildren;
  TacWorld* mWorld = nullptr;
  TacEntityUUID mEntityUUID = TacNullEntityUUID;
  std::list< TacComponent* > mComponents;

  TacRelativeSpace mRelativeSpace;
  bool mInheritParentScale = false;

  v3 mWorldPosition = {};
  m4 mWorldTransform;

  //v3 mLocalPosition = {};
  //v3 mLocalEulerRads = {};
  //v3 mLocalScale = { 1, 1, 1 };

  //m4 mLocalTransform;

  //m4 mWorldTransformNoScale;

  TacString mName;
};

const TacVector< TacNetworkBit > TacEntityBits =
{
  //{ "mPosition", TacOffsetOf( TacEntity, mLocalPosition ), sizeof( float ), 3 },
};

