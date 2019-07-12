#include "space/taccomponent.h"
//#include "space/tacsay.h"
//#include "space/tacterrain.h"


//{
//  TacComponentRegistryEntryIndex::Say,
//  new TacComponentData(
//    "Say",
//    TacSystemType::Graphics,
//    TacSayBits)
//},
//
//TacComponentRegistryEntryIndex::Terrain,
//new TacComponentData(
//  "Terrain",
//  TacSystemType::Physics,
//  TacTerrainBits )

TacComponentRegistry* TacComponentRegistry::Instance()
{
  static TacComponentRegistry registry;
  return &registry;
}

TacComponentRegistryEntry* TacComponentRegistry::RegisterNewEntry()
{
  auto entry = new TacComponentRegistryEntry;
  mEntries.push_back( entry );
  return entry;
}
