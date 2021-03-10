
#pragma once

namespace Tac
{
  struct World;
  struct Camera;
  struct Errors;
  struct Entity;
  struct StringView;
  struct v3;

  void                PrefabSave( World* );
  void                PrefabLoadAtPath( World*, Camera*, String, Errors& );
  void                PrefabLoad( World*, Camera*, Errors& );
  //void                PrefabLoadCamera( Prefab* );
  //void                PrefabLoadCameraVec( Prefab*, StringView, v3& );
  void                PrefabSaveCamera( Camera* );
  void                PrefabRemoveEntityRecursively( Entity* );

}

