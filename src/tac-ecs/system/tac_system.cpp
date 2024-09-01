#include "tac-ecs/system/tac_system.h"

#include "tac-std-lib/containers/tac_fixed_vector.h"


namespace Tac
{
  static FixedVector< SystemInfo, 10 > mEntries;

  //SystemRegistry* SystemRegistry::Instance()
  //{
  //  static SystemRegistry systemRegistry;
  //  return &systemRegistry;
  //}

  SystemInfo* SystemInfo::Register()
  {
    const int index { mEntries.size() };
    mEntries.resize( index + 1 );
    SystemInfo* systemRegistryEntry { &mEntries[ index ] };
    systemRegistryEntry->mIndex = index;
    return systemRegistryEntry;
  }

  //const SystemInfo* SystemRegistry::Find( StringView name ) const
  //{
  //  for( const SystemInfo& systemRegistryEntry : mEntries )
  //    if( systemRegistryEntry.mName == name )
  //      return &systemRegistryEntry;
  //  return nullptr;
  //}

  const SystemInfo* SystemInfo::Iterate::begin() const { return mEntries.begin(); }
  const SystemInfo* SystemInfo::Iterate::end() const   { return mEntries.end(); }

//  static SystemRegistry* Instance();
//  SystemInfo* RegisterNewEntry();
//  //const SystemInfo* Find( const char* ) const;
//  FixedVector< SystemInfo, 10 > mEntries;
//};
} // namespace Tac
