#pragma once

//#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacFixedVector.h"
#include "src/common/tacString.h"
//#include "src/space/tacSpaceTypes.h"

namespace Tac
{


  struct World;
  //struct Component;
  //struct SystemRegistry;
  //struct SystemRegistryEntry;

  struct System
  {
    //virtual const Vector< ComponentRegistryEntryIndex >& GetManagedComponentTypes() = 0;
    //virtual Component* CreateComponent( ComponentRegistryEntryIndex componentType ) = 0;
    //virtual void DestroyComponent( Component* component ) = 0;
    virtual void DebugImgui() {};
    virtual void Update() {};
    //virtual SystemRegistryEntry* GetEntry() = 0;
    //virtual SystemType GetSystemType() = 0;

    World* mWorld = nullptr;
  };


  struct SystemRegistryEntry
  {
    System* ( *mCreateFn )( ) = nullptr;
    String mName;

    void( *mDebugImGui )( System* ) = nullptr;

    // Index of this system in the registry, also the 
    // index of this system in the world systems array
    int mIndex = -1;
  };

  struct SystemRegistry
  {
    static SystemRegistry* Instance();
    SystemRegistryEntry* RegisterNewEntry();
    const SystemRegistryEntry* begin() const { return mEntries.begin(); }
    const SystemRegistryEntry* end() const { return mEntries.end(); }
    const SystemRegistryEntry* Find( StringView ) const;
    FixedVector< SystemRegistryEntry, 10 > mEntries;
  };


}
