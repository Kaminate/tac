#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"


#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct Example
  {
    using Callback = void* ( * )( );

    Example();
    virtual ~Example();

    // converts "foo.png" from the "FooExample" to "assets/example/FooExample/foo.png"
    auto GetFileAssetPath( const char* ) -> AssetPathString;
    void TryCallFn( Callback fn ) { if( fn ) fn(); }
    auto GetWorldspaceKeyboardDir() -> v3;

    virtual void Init()            { TryCallFn( mInitFn ); }
    virtual void Update( Errors& ) { TryCallFn( mUpdateFn ); }
    virtual void Render()          { TryCallFn( mRenderFn ); }
    virtual void Uninit()          { TryCallFn( mUninitFn ); }

    Callback     mInitFn   {};
    Callback     mUpdateFn {};
    Callback     mRenderFn {};
    Callback     mUninitFn {};
    World*       mWorld    {};
    Camera*      mCamera   {};
    const char*  mName     {};
  };

}
