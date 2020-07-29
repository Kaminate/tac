#include "src/space/tacSystem.h"

namespace Tac
{
  SystemRegistry* SystemRegistry::Instance()
  {
    static SystemRegistry systemRegistry;
    return &systemRegistry;
  }

  SystemRegistryEntry* SystemRegistry::RegisterNewEntry()
  {
    const int index = mEntries.size();
    mEntries.resize( index + 1 );
    SystemRegistryEntry* systemRegistryEntry = &mEntries[ index ];;
    systemRegistryEntry->mIndex = index;
    return systemRegistryEntry;
  }

  const SystemRegistryEntry* SystemRegistry::Find( StringView name ) const
  {
    for( const SystemRegistryEntry& systemRegistryEntry : mEntries )
      if( systemRegistryEntry.mName == name )
        return &systemRegistryEntry;
    return nullptr;
  }
}
