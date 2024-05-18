#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_system_window.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"

namespace Tac
{
  static const SystemRegistryEntry* sSystemRegistryEntry;

  static const char* GetNSysPath() { return "SystemWindow.nSys"; }

  CreationSystemWindow* CreationSystemWindow::Instance { nullptr };

  CreationSystemWindow::CreationSystemWindow() { Instance = this; }

  CreationSystemWindow::~CreationSystemWindow()
  {
    SimWindowApi* windowApi{};
    windowApi->DestroyWindow( mWindowHandle );
    Instance = nullptr;
  }

  void CreationSystemWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    const String systemName = SettingsGetString( GetNSysPath(), "" );
    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      if( !StrCmp( entry.mName, systemName.c_str() ) )
        sSystemRegistryEntry = &entry;

    mWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gSystemWindowName );
  }

  void CreationSystemWindow::ImGui()
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mWindowHandle );
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
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGui();
    const v2 size = desktopWindowState->GetSizeV2();
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport(size) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect(size) );
  }


}

