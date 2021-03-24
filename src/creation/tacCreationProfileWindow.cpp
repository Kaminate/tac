#include "src/common/graphics/tacUI2D.h"
#include "src/common/shell/tacShell.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacOS.h"
#include "src/common/profile/tacProfile.h"
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
    DesktopAppDestroyWindow( mDesktopWindowHandle );
  }
  void CreationProfileWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mUI2DDrawData = TAC_NEW UI2DDrawData;
    mDesktopWindowHandle = gCreation.CreateWindow( gProfileWindowName );
  };
  void CreationProfileWindow::ImGuiProfile()
  {
    ImGuiProfileWidget();
  }
  void CreationProfileWindow::ImGui()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiBegin( "Profile Window" );

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
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );
    ImGui();
    mUI2DDrawData->DrawToTexture( viewHandle,
                                  desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }


}

