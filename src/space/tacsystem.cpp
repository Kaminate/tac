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
    auto entry = TAC_NEW SystemRegistryEntry;
    entry->mIndex = mEntries.size();
    mEntries.push_back( entry );
    return entry;
  }
}
