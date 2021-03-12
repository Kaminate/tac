
#pragma once
#include "src/space/tacSpacetypes.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacString.h"
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


  struct Components
  {
    typedef std::list< Component* >::const_iterator ConstIterator;
    void          Add( Component* );
    void          Clear();
    Component*    Remove( const ComponentRegistryEntry* );
    ConstIterator begin() const;
    ConstIterator end()const;
    std::list< Component* > mComponents;
  };

  struct Entity
  {
    ~Entity();

    void              RemoveAllComponents();
    Component*        GetComponent( const ComponentRegistryEntry* );
    const Component*  GetComponent( const ComponentRegistryEntry* ) const;
    bool              HasComponent( const ComponentRegistryEntry* );
    Component*        AddNewComponent( const ComponentRegistryEntry* );
    void              RemoveComponent( const ComponentRegistryEntry* );

    void              DeepCopy( const Entity& );
    void              DebugImgui();
    //void Integrate( float time );
    void              Unparent();
    void              AddChild( Entity* );
    void              Save( Json& );
    void              Load( Json& );

    Entity*           mParent = nullptr;
    Vector< Entity* > mChildren;
    World*            mWorld = nullptr;
    EntityUUID        mEntityUUID = NullEntityUUID;
    Components        mComponents;


    //                Position / Rotation / Scale of the entity relative to its parent.
    //                Used to construct mWorldPosition and mWorldTransform
    RelativeSpace     mRelativeSpace;

    bool              mInheritParentScale = false;

    //                Position of this entity in worldspace.
    //                Computed automatically during the hierarchical world traversl.
    v3                mWorldPosition = {};

    //                World transformation matrix ( aka world matrix ) of this entity.
    //                Computed automatically during the hierarchical world traversl.
    m4                mWorldTransform = m4::Identity();

    //                can this be a tag or smthn
    String            mName;
  };


}

