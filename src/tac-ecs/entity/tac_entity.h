#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-ecs/component/tac_components.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac { struct Json; }

namespace Tac
{
  struct RelativeSpace
  {
    v3 mPosition  {};
    v3 mEulerRads {};
    v3 mScale     { 1, 1, 1 };
  };

#if 0
  RelativeSpace RelativeSpaceFromMatrix( const m4& );
#endif


  struct Entity
  {
    ~Entity();

    void              RemoveAllComponents();
    Component*        GetComponent( const ComponentInfo* );
    const Component*  GetComponent( const ComponentInfo* ) const;
    bool              HasComponent( const ComponentInfo* );
    Component*        AddNewComponent( const ComponentInfo* );
    void              RemoveComponent( const ComponentInfo* );

    void              DeepCopy( const Entity& );
    void              DebugImgui();
    //void Integrate( float time );
    void              Unparent();
    void              AddChild( Entity* );
    Json              Save();
    void              Load( const Json& );

    Entity*           mParent             {};
    Vector< Entity* > mChildren           {};
    World*            mWorld              {};
    EntityUUID        mEntityUUID         { NullEntityUUID };
    Components        mComponents         {};


    //                Position / Rotation / Scale of the entity relative to its parent.
    //                Used to construct mWorldPosition and mWorldTransform
    RelativeSpace     mRelativeSpace      {};

    bool              mInheritParentScale {};
    bool              mActive             { true };
    bool              mStatic             {};

    //                Position of this entity in worldspace.
    //                Computed automatically during the hierarchical world traversl.
    v3                mWorldPosition      {};

    //                World transformation matrix ( aka world matrix ) of this entity.
    //                Computed automatically during the hierarchical world traversl.
    m4                mWorldTransform     { m4::Identity() };

    //                can this be a tag or smthn
    String            mName               {};
  };

  struct EntitiesToDiff{ Entity* mOldEntity; Entity* mNewEntity; };


} // namespace Tac

