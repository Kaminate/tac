#include "taccollider.h"



void TacColliderDebugImgui( TacCollider* collider )
{
  //if( !ImGui::CollapsingHeader( "Collider" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::DragFloat3( "Velocity" , mVelocity.data(), 0.1f );
  //ImGui::DragFloat( "Capsule Radius", &mRadius, 0.1f );
  //ImGui::DragFloat( "Capsule Height", &mTotalHeight, 0.1f );
}
void TacColliderDebugImgui( TacComponent* component )
{
  TacColliderDebugImgui( ( TacCollider* )component );
}
