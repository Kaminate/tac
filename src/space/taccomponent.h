#pragma once
#include "common/tacSerialization.h"
#include "tacspacetypes.h"
#include <map>

struct TacEntity;
struct TacWorld;
struct TacComponentRegistry;
struct TacComponentRegistryEntry;
struct TacSystemRegistry;
struct TacSystemRegistryEntry;

struct TacComponent
{
  virtual ~TacComponent() = default;
  virtual void TacDebugImgui() {};
  virtual void PreReadDifferences() {};
  virtual void PostReadDifferences() {};
  virtual TacComponentRegistryEntry* GetEntry() = 0;
  TacEntity* mEntity = nullptr;
};

struct TacComponentRegistryEntry
{
  // Used for
  // - debugging network bits
  // - prefab serialization
  const char* mName = nullptr;

  TacComponent* ( *mCreateFn )( TacWorld* ) = nullptr;
  TacSystemRegistryEntry* mSystemRegistryEntry = nullptr;
  TacVector< TacNetworkBit > mNetworkBits;
};

struct TacComponentRegistry
{
  static TacComponentRegistry* Instance();
  TacComponentRegistryEntry* RegisterNewEntry();

  // I wonder if these can be out of sync between different builds of the exe
  // or between server/clients
  // maybe it should be sorted by entry name or something?
  TacVector< TacComponentRegistryEntry* > mEntries;
};
