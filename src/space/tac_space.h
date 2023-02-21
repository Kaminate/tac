
#pragma once

namespace Tac
{

  void SpaceInit();

  // Components
  struct Collider;
  struct Model;
  struct Skybox;
  struct Light;
  struct Terrain;

  // Entity-Component Systems
  struct Component;
  struct ComponentRegistryEntry;
  struct Entity;
  struct RelativeSpace;
  struct System;
  struct SystemRegistryEntry;

  // Systems
  struct Graphics;
  struct Physics;

  struct Player;
  struct OtherPlayer;
  struct Ghost;
  struct User;
  struct World;

  // Client-server
  struct ClientData;
  struct ServerData;

  // Scripting
  struct ScriptMsg;
  struct ScriptCallbackData;
  struct ScriptThread;
  struct ScriptRoot;

}

