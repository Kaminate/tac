#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

namespace Tac
{
  static ComponentInfo* sEntry;

  TAC_META_REGISTER_STRUCT_BEGIN( Collider );
  TAC_META_REGISTER_STRUCT_MEMBER( mVelocity );
#if 0
  TAC_META_REGISTER_STRUCT_MEMBER( mRadius );
  TAC_META_REGISTER_STRUCT_MEMBER( mTotalHeight );
#endif
  TAC_META_REGISTER_STRUCT_END( Collider );

  auto Collider::GetCollider( Entity* entity ) -> Collider*
  {
    return ( Collider* )entity->GetComponent( sEntry );
  }

  auto Collider::GetEntry() const -> const ComponentInfo* { return sEntry; }

  auto Collider::CreateCollider( World* world ) -> Collider*
  {
    return Physics::GetSystem( world )->CreateCollider();
  }

  static void DestroyColliderComponent( World* world, Component* component )
  {
    Physics::GetSystem( world )->DestroyCollider( ( Collider* )component );
  }

  static auto CreateColliderComponent( World* world ) -> Component*
  {
     return Collider::CreateCollider( world );
  }

  static void DebugComponent(  Component* component  )
  {
    Collider* collider{ ( Collider* )component  };
    ImGuiDragFloat3( "Velocity" , collider->mVelocity.data() );
#if 0
    ImGuiDragFloat( "Capsule Radius", &collider->mRadius  );
    ImGuiDragFloat( "Capsule Height", &collider->mTotalHeight );
#endif
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
      .mMetaType     { &GetMetaType<Collider>() },
    };
  }

}

