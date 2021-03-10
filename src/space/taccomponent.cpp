#include "src/space/tacComponent.h"
#include "src/common/tacString.h"

namespace Tac
{
  // I wonder if these can be out of sync between different builds of the exe
  // or between server/clients
  // maybe it should be sorted by entry name or something?

  static FixedVector< ComponentRegistryEntry, 10 > mEntries;

  ComponentRegistryEntry* ComponentRegistryIterator::begin() { return mEntries.begin(); }
  ComponentRegistryEntry* ComponentRegistryIterator::end() { return mEntries.end(); }

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

  int                     ComponentRegistry_GetComponentCount()
  {
    return mEntries.size();
  }
  ComponentRegistryEntry* ComponentRegistry_GetComponentAtIndex( int i )
  {
    return &mEntries[ i ];
  }

}

