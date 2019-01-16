#include "taccollider.h"
#include "common/imgui.h"
void TacCollider::TacDebugImgui()
{
  if( !ImGui::CollapsingHeader( "Collider" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::DragFloat3( "Velocity" , mVelocity.data(), 0.1f );
  ImGui::DragFloat( "Capsule Radius", &mRadius, 0.1f );
  ImGui::DragFloat( "Capsule Height", &mTotalHeight, 0.1f );
}
