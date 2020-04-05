
#pragma once
#include "src/space/tacSpacetypes.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include <list>

namespace Tac
{
struct World;
struct Component;
struct ComponentRegistryEntry;
struct Json;

struct RelativeSpace
{
  v3 mPosition = {};
  v3 mEulerRads = {};
  v3 mScale = { 1, 1, 1 };
};

struct Entity
{
  ~Entity();

  void RemoveAllComponents();
  
  Component* GetComponent( ComponentRegistryEntry* );
  const Component* GetComponent( ComponentRegistryEntry* ) const;
  bool HasComponent( ComponentRegistryEntry* );
  Component* AddNewComponent( ComponentRegistryEntry* );
  void RemoveComponent( ComponentRegistryEntry* );

  void DeepCopy( const Entity& );
  void DebugImgui();
  //void Integrate( float time );
  void Unparent();
  void AddChild( Entity* child );
  void Save( Json& entityJson );
  void Load( Json& entityJson );

  Entity* mParent = nullptr;
  Vector< Entity* > mChildren;
  World* mWorld = nullptr;
  EntityUUID mEntityUUID = NullEntityUUID;
  std::list< Component* > mComponents;

  RelativeSpace mRelativeSpace;
  bool mInheritParentScale = false;

  v3 mWorldPosition = {};
  m4 mWorldTransform = m4::Identity();

  //v3 mLocalPosition = {};
  //v3 mLocalEulerRads = {};
  //v3 mLocalScale = { 1, 1, 1 };

  //m4 mLocalTransform;

  //m4 mWorldTransformNoScale;

  String mName;
};

const Vector< NetworkBit > EntityBits =
{
  //{ "mPosition", OffsetOf( Entity, mLocalPosition ), sizeof( float ), 3 },
};


}

