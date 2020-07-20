
#include "src/space/tacComponent.h"
namespace Tac
{


//{
//  ComponentRegistryEntryIndex::Say,
//  new ComponentData(
//    "Say",
//    SystemType::Graphics,
//    SayBits)
//},
//
//ComponentRegistryEntryIndex::Terrain,
//new ComponentData(
//  "Terrain",
//  SystemType::Physics,
//  TerrainBits )

ComponentRegistry* ComponentRegistry::Instance()
{
  static ComponentRegistry registry;
  return &registry;
}

ComponentRegistryEntry* ComponentRegistry::RegisterNewEntry()
{
  auto entry = new ComponentRegistryEntry;
  mEntries.push_back( entry );
  return entry;
}

ComponentRegistryEntry* ComponentRegistry::FindEntryNamed( StringView name )
{
  for( auto entry  : mEntries )
  {
    if( entry->mName == name )
    {
      return entry;
    }
  }

  return nullptr;
}

}

