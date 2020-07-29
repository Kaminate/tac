
#include "src/space/tacComponent.h"
namespace Tac
{
  ComponentRegistry* ComponentRegistry::Instance()
  {
    static ComponentRegistry registry;
    return &registry;
  }

  ComponentRegistryEntry* ComponentRegistry::RegisterNewEntry()
  {
    mEntries.push_back( ComponentRegistryEntry() );
    return &mEntries.back();
  }

  ComponentRegistryEntry* ComponentRegistry::FindEntryNamed( StringView name )
  {
    for( ComponentRegistryEntry& entry : mEntries )
    {
      if( entry.mName == name )
      {
        return &entry;
      }
    }

    return nullptr;
  }
}

