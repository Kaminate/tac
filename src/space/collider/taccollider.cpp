#include "src/space/collider/taccollider.h"
#include "src/space/tacentity.h"
#include "src/space/physics/tacphysics.h"
namespace Tac
{


  Collider* Collider::GetCollider( Entity* entity )
  {
    return ( Collider* )entity->GetComponent( Collider::ColliderComponentRegistryEntry );
  }

  ComponentRegistryEntry* Collider::GetEntry() const
  {
    return Collider::ColliderComponentRegistryEntry;
  }

  static Component* CreateColliderComponent( World* world )
  {
    return Physics::GetSystem( world )->CreateCollider();
  }

  static void DestroyColliderComponent( World* world, Component* component )
  {
    Physics::GetSystem( world )->DestroyCollider( ( Collider* )component );
  }
  ComponentRegistryEntry* Collider::ColliderComponentRegistryEntry;

  void ColliderDebugImgui( Component* );
  void RegisterColliderComponent()
  {
    NetworkBits networkBits;
    networkBits.Add( { "mVelocity",    TAC_OFFSET_OF( Collider, mVelocity ),    sizeof( float ), 3 } );
    networkBits.Add( { "mRadius",      TAC_OFFSET_OF( Collider, mRadius ),      sizeof( float ), 1 } );
    networkBits.Add( { "mTotalHeight", TAC_OFFSET_OF( Collider, mTotalHeight ), sizeof( float ), 1 } );
    Collider::ColliderComponentRegistryEntry = ComponentRegistry_RegisterComponent();
    Collider::ColliderComponentRegistryEntry->mName = "Collider";
    Collider::ColliderComponentRegistryEntry->mNetworkBits = networkBits;
    Collider::ColliderComponentRegistryEntry->mCreateFn = CreateColliderComponent;
    Collider::ColliderComponentRegistryEntry->mDestroyFn = DestroyColliderComponent;
    Collider::ColliderComponentRegistryEntry->mDebugImguiFn = ColliderDebugImgui;
  }

}

