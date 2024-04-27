#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/shell/tac_shell.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/level_editor/tac_level_editor_system_window.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_system.h"
#include "space/world/tac_world.h"
#include "src/common/shell/tac_shell_timestep.h"

namespace Tac
{
  static const SystemRegistryEntry* sSystemRegistryEntry;

  static const char* GetNSysPath() { return "SystemWindow.nSys"; }

  CreationSystemWindow* CreationSystemWindow::Instance { nullptr };

  CreationSystemWindow::CreationSystemWindow() { Instance = this; }

  CreationSystemWindow::~CreationSystemWindow()
  {
    DesktopApp::GetInstance()->DestroyWindow( mDesktopWindowHandle );
    Instance = nullptr;
  }

  void CreationSystemWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    const String systemName = SettingsGetString( GetNSysPath(), "" );
    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      if( !StrCmp( entry.mName, systemName.c_str() ) )
        sSystemRegistryEntry = &entry;

    mDesktopWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gSystemWindowName );
  }

  void CreationSystemWindow::ImGui()
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiSetNextWindowStretch();



    ImGuiBegin( "System Window" );


    if( ImGuiCollapsingHeader( "Select System" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( const SystemRegistryEntry& systemRegistryEntry : SystemRegistryIterator() )
      {
        if( ImGuiButton( systemRegistryEntry.mName ) )
        {
          sSystemRegistryEntry = &systemRegistryEntry;
          SettingsSetString( GetNSysPath(), systemRegistryEntry.mName );
        }
        if( sSystemRegistryEntry == &systemRegistryEntry )
        {
          ImGuiSameLine();
          ImGuiText( "<-- currently selected" );
        }
      }
    }

    if( sSystemRegistryEntry &&
        sSystemRegistryEntry->mDebugImGui &&
        ImGuiCollapsingHeader( ShortFixedString::Concat( sSystemRegistryEntry->mName, " Debug" ) ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      System* system = gCreation.mWorld->GetSystem( sSystemRegistryEntry );
      sSystemRegistryEntry->mDebugImGui( system );
    }

    mCloseRequested |= ImGuiButton( "Close window" );

    // to force directx graphics specific window debugging
    //if( ImGuiButton( "close window" ) )
    //{
    //  mDesktopWindow->mRequestDeletion = true;
    //}
    ImGuiEnd();
  }

  void CreationSystemWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;
    DesktopApp::GetInstance()->ResizeControls( mDesktopWindowHandle );
    DesktopApp::GetInstance()->MoveControls( mDesktopWindowHandle );
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGui();
    const v2 size = desktopWindowState->GetSizeV2();
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport(size) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect(size) );
  }


}

