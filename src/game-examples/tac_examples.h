#pragma once
#include "src/common/string/tac_string.h"

namespace Tac
{
  struct Errors;
  struct World;
  struct Camera;

  struct Example
  {
    Example();
    virtual ~Example();
    virtual void Update( Errors& ){};
    v3 GetWorldspaceKeyboardDir();
    World* mWorld;
    Camera* mCamera;
    const char* mName;
  };

  void ExampleRegistryAdd( const char* exampleName, Example*(*)() );
  void ExampleRegistryPopulate();

}
