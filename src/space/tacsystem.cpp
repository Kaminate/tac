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
  auto entry = new SystemRegistryEntry;
  entry->mIndex = mEntries.size();
  mEntries.push_back( entry );
  return entry;
}
}
