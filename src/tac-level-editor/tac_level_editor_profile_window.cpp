#include "tac_level_editor_profile_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/window/tac_window_handle.h"

#include "tac-level-editor/tac_level_editor.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  CreationProfileWindow* CreationProfileWindow::Instance { nullptr };
  CreationProfileWindow::CreationProfileWindow()
  {
    Instance = this;
  }

  CreationProfileWindow::~CreationProfileWindow()
  {
    Instance = nullptr;
    //TAC_DELETE mUI2DDrawData;
    SimWindowApi* windowApi{};
    windowApi->DestroyWindow( mWindowHandle );
  }

  void CreationProfileWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //mUI2DDrawData = TAC_NEW UI2DDrawData;
    mWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gProfileWindowName );
  }

  void CreationProfileWindow::ImGui()
  {
    TAC_PROFILE_BLOCK;
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowHandle( mWindowHandle );
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
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const v2 size = desktopWindowState->GetSizeV2();

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport(size) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect(size) );
    ImGui();
    //TAC_CALL(mUI2DDrawData->DrawToTexture( viewHandle,
    //                              desktopWindowState->mWidth,
    //                              desktopWindowState->mHeight,
    //                              errors ) );
  }


} // namespace Tac

