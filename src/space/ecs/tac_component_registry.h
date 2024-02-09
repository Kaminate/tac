#pragma once

#include "src/common/dataprocess/tac_serialization.h" // NetworkBits
#include "src/common/tac_core.h"
#include "space/tac_space.h"

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
    const char*            mName = nullptr;

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
  //  Iterator begin() { return { 0 }; }
  //  Iterator end() { return { ComponentRegistry_GetComponentCount() }; }
  //};


  int                     ComponentRegistry_GetComponentCount();
  ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int );
  ComponentRegistryEntry* ComponentRegistry_RegisterComponent();
  ComponentRegistryEntry* ComponentRegistry_FindComponentByName( const char* );

}

