#pragma once

#include "common/containers/tacVector.h"
#include "space/tacspacetypes.h"

struct TacWorld;
struct TacComponent;
struct TacSystemRegistry;
struct TacSystemRegistryEntry;

struct TacSystem
{
  //virtual const TacVector< TacComponentRegistryEntryIndex >& GetManagedComponentTypes() = 0;
  //virtual TacComponent* CreateComponent( TacComponentRegistryEntryIndex componentType ) = 0;
  //virtual void DestroyComponent( TacComponent* component ) = 0;
  virtual void DebugImgui(){};
  virtual void Update(){};
  //virtual TacSystemRegistryEntry* GetEntry() = 0;
  //virtual TacSystemType GetSystemType() = 0;

  TacWorld* mWorld = nullptr;
};


struct TacSystemRegistryEntry
{
  TacSystem* ( *mCreateFn )( ) = nullptr;
  int mIndex = -1;
};

struct TacSystemRegistry
{
  static TacSystemRegistry* Instance();
  TacSystemRegistryEntry* RegisterNewEntry();
  TacVector< TacSystemRegistryEntry* > mEntries;
};


