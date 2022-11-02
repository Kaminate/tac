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
    virtual const char* GetName() const = 0;
    World* mWorld;
    Camera* mCamera;
  };

  void ExampleRegistryAdd( const char* exampleName, Example*(*)() );
  void ExampleRegistryPopulate();

}
