#include "src/space/collider/tac_collider.h"
#include "src/space/tac_entity.h"
#include "src/space/physics/tac_physics.h"

namespace Tac
{
  static ComponentRegistryEntry* sEntry;

  Collider*                     Collider::GetCollider( Entity* entity )
  {
    return ( Collider* )entity->GetComponent( sEntry );
  }

  const ComponentRegistryEntry* Collider::GetEntry() const
  {
    return sEntry;
  }

  Collider*                     Collider::CreateCollider( World* world )
  {
    return Physics::GetSystem( world )->CreateCollider();

  }

  static void       DestroyColliderComponent( World* world, Component* component )
  {
    Physics::GetSystem( world )->DestroyCollider( ( Collider* )component );
  }


  void ColliderDebugImgui( Collider* );
  void RegisterColliderComponent()
  {
    NetworkBits networkBits;
    networkBits.Add( { "mVelocity",    (int)TAC_OFFSET_OF( Collider, mVelocity ),    sizeof( float ), 3 } );
    networkBits.Add( { "mRadius",      (int)TAC_OFFSET_OF( Collider, mRadius ),      sizeof( float ), 1 } );
    networkBits.Add( { "mTotalHeight", (int)TAC_OFFSET_OF( Collider, mTotalHeight ), sizeof( float ), 1 } );
    sEntry = ComponentRegistry_RegisterComponent();
    sEntry->mName = "Collider";
    sEntry->mNetworkBits = networkBits;
    sEntry->mCreateFn = []( World* world ){ return ( Component* ) Collider::CreateCollider( world ); };
    sEntry->mDestroyFn = DestroyColliderComponent;
    sEntry->mDebugImguiFn =
      []( Component* component ){ ColliderDebugImgui( ( Collider* )component ); };
  }

}

