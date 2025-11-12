
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
  struct Material;
  struct CameraComponent;

  // Entity-Component Systems
  struct Component;
  struct ComponentInfo;
  struct Entity;
  struct RelativeSpace;
  struct System;
  struct SystemInfo;

  // Systems
  struct Graphics;
  struct Physics;

  // ...
  struct Player;
  struct OtherPlayer;
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

