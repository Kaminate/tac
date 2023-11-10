#include "src/space/tac_system.h"

#include "src/common/containers/tac_fixed_vector.h"


namespace Tac
{
  static FixedVector< SystemRegistryEntry, 10 > mEntries;

  //SystemRegistry* SystemRegistry::Instance()
  //{
  //  static SystemRegistry systemRegistry;
  //  return &systemRegistry;
  //}

  SystemRegistryEntry* SystemRegisterNewEntry()
  {
    const int index = mEntries.size();
    mEntries.resize( index + 1 );
    SystemRegistryEntry* systemRegistryEntry = &mEntries[ index ];
    systemRegistryEntry->mIndex = index;
    return systemRegistryEntry;
  }

  //const SystemRegistryEntry* SystemRegistry::Find( StringView name ) const
  //{
  //  for( const SystemRegistryEntry& systemRegistryEntry : mEntries )
  //    if( systemRegistryEntry.mName == name )
  //      return &systemRegistryEntry;
  //  return nullptr;
  //}

  const SystemRegistryEntry* SystemRegistryIterator::begin() const { return mEntries.begin(); }
  const SystemRegistryEntry* SystemRegistryIterator::end() const { return mEntries.end(); }

//  static SystemRegistry* Instance();
//  SystemRegistryEntry* RegisterNewEntry();
//  //const SystemRegistryEntry* Find( const char* ) const;
//  FixedVector< SystemRegistryEntry, 10 > mEntries;
//};
}
