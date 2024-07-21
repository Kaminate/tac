#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // NetVars
#include "tac-ecs/tac_space.h"

namespace Tac { struct Json; }
namespace Tac
{
  struct ComponentRegistryEntry
  {
    typedef Component*     ComponentCreateFn( World* );
    typedef void           ComponentDestroyFn( World*, Component* );
    typedef void           ComponentDebugImguiFn( Component* );
    typedef void           ComponentSaveFn( Json&, Component* );
    typedef void           ComponentLoadFn( Json&, Component* );
    int                    GetIndex() const;

    //                     Used for
    //                     - debugging network bits
    //                     - prefab serialization
    const char*            mName         {};

    //                     Used to create components at runtime
    //                     ( from prefabs, or hardcode, or in editor, or whenever )
    //                     Component* ( *mCreateFn )( World* ) = nullptr;
    ComponentCreateFn*     mCreateFn     {};
    ComponentDestroyFn*    mDestroyFn    {};
    ComponentDebugImguiFn* mDebugImguiFn {};
    ComponentSaveFn*       mSaveFn       {};
    ComponentLoadFn*       mLoadFn       {};


    //                     Used for serializing components over the network
    NetVars                mNetVars;
  };

  struct ComponentRegistryIterator
  {
    ComponentRegistryEntry* begin();
    ComponentRegistryEntry* end();
  };

  // Index of a registered component in the ComponentRegistryEntry
  //enum ComponentRegistryIndex : int {};

  //struct ComponentRegistryIndexes
  //{
  //  struct Iterator
  //  {
  //    ComponentRegistryIndex operator*() { return ComponentRegistryIndex{ mIndex }; }
  //    void operator++() { mIndex++; }
  //    int mIndex;
  //  };
  //  Iterator begin() { return {}; }
  //  Iterator end() { return { ComponentRegistry_GetComponentCount() }; }
  //};


  int                     ComponentRegistry_GetComponentCount();
  ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int );
  ComponentRegistryEntry* ComponentRegistry_RegisterComponent();
  ComponentRegistryEntry* ComponentRegistry_FindComponentByName( const char* );

}

