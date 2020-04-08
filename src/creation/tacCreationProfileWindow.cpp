
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacShell.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacOS.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/profile/tacProfileImGui.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationProfileWindow.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSystem.h"
#include "src/shell/tacDesktopApp.h"

namespace Tac
{
CreationProfileWindow::~CreationProfileWindow()
{
  delete mUI2DDrawData;
}
void CreationProfileWindow::Init( Errors& errors )
{
  mUI2DDrawData = new UI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
};
void CreationProfileWindow::ImGuiProfile()
{
  ProfileSystem* profileSystem = ProfileSystem::Instance;
  ImGuiProfileWidget( profileSystem->mLastFrame );
}
void CreationProfileWindow::ImGui()
{
  SetCreationWindowImGuiGlobals( mDesktopWindow, mUI2DDrawData );
  ImGuiBegin( "Profile Window", {} );

  ImGuiText( "i am the profile window" );

  // to force directx graphics specific window debugging
  if( ImGuiButton( "close window" ) )
  {
    mDesktopWindow->mRequestDeletion = true;
  }

  ImGuiProfile();



  ImGuiEnd();
}
void CreationProfileWindow::Update( Errors& errors )
{
  ;
  mDesktopWindow->SetRenderViewDefaults();
  ImGui();
  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}


}

