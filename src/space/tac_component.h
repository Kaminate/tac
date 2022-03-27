#pragma once

#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/tac_serialization.h"
//#include "src/common/string/tac_string.h"
//#include "src/space/tac_space_types.h"

namespace Tac
{
  struct Entity;
  struct World;
  struct ComponentRegistryEntry;
  struct SystemRegistry;
  struct SystemRegistryEntry;
  struct Json;

  struct Component
  {
    virtual ~Component() = default;
    virtual void                          PreReadDifferences() {};
    virtual void                          PostReadDifferences() {};
    virtual const ComponentRegistryEntry* GetEntry() const = 0;
    Entity*                               mEntity = nullptr;
  };


  struct ComponentRegistryEntry
  {
    typedef Component*     ComponentCreateFn( World* );
    typedef void           ComponentDestroyFn( World*, Component* );
    typedef void           ComponentDebugImguiFn( Component* );
    typedef void           ComponentSaveFn( Json&, Component* );
    typedef void           ComponentLoadFn( Json&, Component* );

    //                     Used for
    //                     - debugging network bits
    //                     - prefab serialization
    const char*            mName;

    //                     Used to create components at runtime
    //                     ( from prefabs, or hardcode, or in editor, or whenever )
    //                     Component* ( *mCreateFn )( World* ) = nullptr;
    ComponentCreateFn*     mCreateFn = nullptr;
    ComponentDestroyFn*    mDestroyFn = nullptr;
    ComponentDebugImguiFn* mDebugImguiFn = nullptr;
    ComponentSaveFn*       mSaveFn = nullptr;
    ComponentLoadFn*       mLoadFn = nullptr;


    //                     Used for serializing components over the network
    NetworkBits            mNetworkBits;
  };

  struct ComponentRegistryIterator
  {
    ComponentRegistryEntry* begin();
    ComponentRegistryEntry* end();
  };

  int                     ComponentRegistry_GetComponentCount();
  ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int );
  ComponentRegistryEntry* ComponentRegistry_RegisterComponent();
  ComponentRegistryEntry* ComponentRegistry_FindComponentByName( const char* );

}

