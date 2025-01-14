#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/component/tac_component_registry.h"

namespace Tac
{
  static ComponentInfo* sEntry;

  Collider*                     Collider::GetCollider( Entity* entity )
  {
    return ( Collider* )entity->GetComponent( sEntry );
  }

  const ComponentInfo* Collider::GetEntry() const
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

  static Component* CreateColliderComponent( World* world )
  {
     return Collider::CreateCollider( world );
  }

  void ColliderDebugImgui( Collider* );

  static void DebugComponent(  Component* component  )
  {
       ColliderDebugImgui( ( Collider* )component );
  }


  void Collider::RegisterComponent()
  {
#if 0
    NetVars networkBits;
    networkBits.Add( { "mVelocity",    ( int )TAC_OFFSET_OF( Collider, mVelocity ),    sizeof( float ), 3 } );
    networkBits.Add( { "mRadius",      ( int )TAC_OFFSET_OF( Collider, mRadius ),      sizeof( float ), 1 } );
    networkBits.Add( { "mTotalHeight", ( int )TAC_OFFSET_OF( Collider, mTotalHeight ), sizeof( float ), 1 } );
#endif
    *( sEntry = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName         { "Collider" },
      //sEntry->mNetVars = networkBits;
      .mCreateFn     { CreateColliderComponent },
      .mDestroyFn    { DestroyColliderComponent },
      .mDebugImguiFn { DebugComponent },
    };
  }

}

