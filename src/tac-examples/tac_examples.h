#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct Example
  {
    struct UpdateParams
    {
      const SimKeyboardApi mKeyboardApi;
      const SimWindowApi mWindowApi;
    };

    Example();
    virtual ~Example();
    virtual void Init()                          { if( mInitFn ) mInitFn(); }
    virtual void Update( UpdateParams, Errors& ) { if( mUpdateFn ) mUpdateFn(); }
    virtual void Render()                        { if( mRenderFn ) mRenderFn(); }
    virtual void Uninit()                        { if( mUninitFn ) mUninitFn(); }
    v3                   GetWorldspaceKeyboardDir();

    void* ( *mInitFn )( ) {};
    void* ( *mUpdateFn )( ) {};
    void* ( *mRenderFn )( ) {};
    void* ( *mUninitFn )( ) {};
    World*                mWorld       {};
    Camera*               mCamera      {};
    const char*           mName        {};
  };

}
