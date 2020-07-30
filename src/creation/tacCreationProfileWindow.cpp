
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
  CreationProfileWindow* CreationProfileWindow::Instance = nullptr;
  CreationProfileWindow::CreationProfileWindow()
  {
    Instance = this;
  }
  CreationProfileWindow::~CreationProfileWindow()
  {
    Instance = nullptr;
    delete mUI2DDrawData;
  }
  void CreationProfileWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mUI2DDrawData = TAC_NEW UI2DDrawData;
    mDesktopWindowHandle = Creation::Instance->CreateWindow( gProfileWindowName );
  };
  void CreationProfileWindow::ImGuiProfile()
  {
    ProfileSystem* profileSystem = ProfileSystem::Instance;
    ImGuiProfileWidget( profileSystem->mLastFrame );
  }
  void CreationProfileWindow::ImGui()
  {
    DesktopWindowState* desktopWindowState = FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    SetCreationWindowImGuiGlobals( desktopWindowState, mUI2DDrawData );
    //                               mUI2DDrawData,
    //                               mDesktopWindowState.mWidth,
    //                               mDesktopWindowState.mHeight );
    ImGuiBegin( "Profile Window", {} );

    ImGuiText( "i am the profile window" );

    //// to force directx graphics specific window debugging
    //if( ImGuiButton( "close window" ) )
    //{
    //  mDesktopWindow->mRequestDeletion = true;
    //}

    ImGuiProfile();



    ImGuiEnd();
  }
  void CreationProfileWindow::Update( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    Creation::WindowFramebufferInfo* info =
      Creation::Instance->FindWindowFramebufferInfo( mDesktopWindowHandle );
    Render::SetViewFramebuffer( ViewIdProfileWindow, info->mFramebufferHandle );
    Render::SetViewport( ViewIdProfileWindow, Viewport( w, h ) );
    Render::SetViewScissorRect( ViewIdProfileWindow, ScissorRect( w, h ) );
    ImGui();
    mUI2DDrawData->DrawToTexture( desktopWindowState->mWidth ,
                                  desktopWindowState->mHeight ,
                                  ViewIdProfileWindow,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }


}

