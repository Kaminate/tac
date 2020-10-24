
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
#include "src/shell/tacDesktopWindowGraphics.h"

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
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    ImGuiBegin( "Profile Window", {}, mDesktopWindowHandle );

    ImGuiText( "i am the profile window" );
    ImGuiText( "i am... inevitable" );

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
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, ScissorRect( w, h ) );
    ImGui();
    mUI2DDrawData->DrawToTexture( viewHandle,
                                  desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }


}

