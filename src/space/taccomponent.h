#pragma once


#include "src/common/containers/tacFixedVector.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacString.h"
#include "src/space/tacSpaceTypes.h"

namespace Tac
{
  struct Entity;
  struct World;
  struct ComponentRegistry;
  struct ComponentRegistryEntry;
  struct SystemRegistry;
  struct SystemRegistryEntry;
  struct Json;

  struct Component
  {
    virtual ~Component() = default;
    virtual void PreReadDifferences() {};
    virtual void PostReadDifferences() {};
    virtual ComponentRegistryEntry* GetEntry() = 0;
    Entity* mEntity = nullptr;
  };

  struct ComponentRegistryEntry
  {
    // Used for
    // - debugging network bits
    // - prefab serialization
    String mName;

    // Used to create components at runtime
    // ( from prefabs, or hardcode, or in editor, or whenever )
    Component* ( *mCreateFn )( World* ) = nullptr;

    void( *mDestroyFn )( World*, Component* ) = nullptr;
    void( *mDebugImguiFn )( Component* ) = nullptr;

    void( *mSaveFn )( Json&, Component* ) = nullptr;
    void( *mLoadFn )( Json&, Component* ) = nullptr;

    // Used for what?
    //SystemRegistryEntry* mSystemRegistryEntry = nullptr;

    // Used for serializing components over the network
    Vector< NetworkBit > mNetworkBits;
  };

  struct ComponentRegistry
  {
    static ComponentRegistry* Instance();
    ComponentRegistryEntry* RegisterNewEntry();
    ComponentRegistryEntry* FindEntryNamed( StringView );

    // I wonder if these can be out of sync between different builds of the exe
    // or between server/clients
    // maybe it should be sorted by entry name or something?
    FixedVector< ComponentRegistryEntry, 10 > mEntries;
  };
}

