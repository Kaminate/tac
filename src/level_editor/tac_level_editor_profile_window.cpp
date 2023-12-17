#include "src/level_editor/tac_level_editor_profile_window.h" // self-inc

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"
#include "src/common/error/tac_error_handling.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_system.h"
#include "space/world/tac_world.h"

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
    mDesktopWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gProfileWindowName );
  }

  void CreationProfileWindow::ImGui()
  {
    TAC_PROFILE_BLOCK;
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    
    DesktopAppMoveControls( mDesktopWindowHandle );

    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiBegin( "Profile Window" );
    mCloseRequested |= ImGuiButton( "Close Window" );

    //// to force directx graphics specific window debugging
    //if( ImGuiButton( "close window" ) )
    //{
    //  mDesktopWindow->mRequestDeletion = true;
    //}

    ImGuiProfileWidget();
    ImGuiEnd();
  }

  void CreationProfileWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const v2 size = desktopWindowState->GetSizeV2();

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport(size) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect(size) );
    ImGui();
    TAC_CALL(mUI2DDrawData->DrawToTexture( viewHandle,
                                  desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  errors ) );
  }


} // namespace Tac

