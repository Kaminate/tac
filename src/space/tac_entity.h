#pragma once

#include "src/space/tac_space_types.h"
#include "src/space/tac_space.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_matrix4.h"
#include "src/common/string/tac_string.h"

#include <list>

namespace Tac
{
  struct RelativeSpace
  {
    v3 mPosition = {};
    v3 mEulerRads = {};
    v3 mScale = { 1, 1, 1 };
  };

  RelativeSpace RelativeSpaceFromMatrix( const m4& );


  struct Components
  {
    //typedef std::list< Component* >::const_iterator ConstIterator;
    void          Add( Component* );
    void          Clear();
    Component*    Remove( const ComponentRegistryEntry* );
    auto          begin() const { return mComponents.begin(); };
    auto          end() const { return mComponents.end(); }
    //ConstIterator begin() const;
    //ConstIterator end()const;
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
    bool              mActive = true;

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

