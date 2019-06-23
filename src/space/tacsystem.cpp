#include "tacsystem.h"

TacSystemRegistry* TacSystemRegistry::Instance()
{
  static TacSystemRegistry systemRegistry;
  return &systemRegistry;
}
TacSystemRegistryEntry* TacSystemRegistry::RegisterNewEntry()
{
  auto entry = new TacSystemRegistryEntry;
  entry->mIndex = mEntries.size();
  mEntries.push_back( entry );
  return entry;
}
