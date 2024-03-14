#include "tac_component_registry.h" // self-inc

#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  // I wonder if these can be out of sync between different builds of the exe
  // or between server/clients
  // maybe it should be sorted by entry name or something?

  static FixedVector< ComponentRegistryEntry, 10 > mEntries;

  ComponentRegistryEntry* ComponentRegistryIterator::begin() { return mEntries.begin(); }
  ComponentRegistryEntry* ComponentRegistryIterator::end() { return mEntries.end(); }

  int                    ComponentRegistryEntry::GetIndex() const
  {
    return ( int )( this - mEntries.data() );
  }

  ComponentRegistryEntry* ComponentRegistry_RegisterComponent()
  {
    mEntries.push_back( ComponentRegistryEntry() );
    return &mEntries.back();
  }

  ComponentRegistryEntry* ComponentRegistry_FindComponentByName( const char* name )
  {
    for( ComponentRegistryEntry& entry : mEntries )
      if( !StrCmp( entry.mName, name ) )
        return &entry;
    return nullptr;
  }

  int                     ComponentRegistry_GetComponentCount() { return mEntries.size(); }

  ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int i ) { return &mEntries[ i ]; }

}

