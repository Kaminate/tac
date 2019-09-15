#pragma once
#include "common/tacSerialization.h"
#include "common/tacString.h"
#include "tacspacetypes.h"
#include <map>

struct TacEntity;
struct TacWorld;
struct TacComponentRegistry;
struct TacComponentRegistryEntry;
struct TacSystemRegistry;
struct TacSystemRegistryEntry;
struct TacJson;

struct TacComponent
{
  virtual ~TacComponent() = default;
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
  TacString mName;

  // Used to create components at runtime
  // ( from prefabs, or hardcode, or in editor, or whenever )
  TacComponent* ( *mCreateFn )( TacWorld* ) = nullptr;

  void ( *mDestroyFn )( TacWorld*, TacComponent* ) = nullptr;
  void ( *mDebugImguiFn )( TacComponent* ) = nullptr;

  void ( *mSaveFn )( TacJson&, TacComponent* ) = nullptr;
  void ( *mLoadFn )( TacJson&, TacComponent* ) = nullptr;

  // Used for what?
  //TacSystemRegistryEntry* mSystemRegistryEntry = nullptr;

  // Used for serializing components over the network
  TacVector< TacNetworkBit > mNetworkBits;
};

struct TacComponentRegistry
{
  static TacComponentRegistry* Instance();
  TacComponentRegistryEntry* RegisterNewEntry();
  TacComponentRegistryEntry* FindEntryNamed( const TacString& name );

  // I wonder if these can be out of sync between different builds of the exe
  // or between server/clients
  // maybe it should be sorted by entry name or something?
  TacVector< TacComponentRegistryEntry* > mEntries;
};


