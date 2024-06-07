#include "tac_level_editor_system_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac
{
  static const SystemRegistryEntry* sSystemRegistryEntry{};
  static const char*                nSysPath{ "SystemWindow.nSys" };
  static bool                       sInitialized;

  static void Initialize( SettingsNode mSettingsNode  )
  {
    const String systemName{ mSettingsNode.GetChild( nSysPath ).GetValueWithFallback( "" ) };
    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      if( entry.mName == systemName.c_str() )
        sSystemRegistryEntry = &entry;
  }

  void CreationSystemWindow::Update( SettingsNode mSettingsNode )
  {
    Initialize( mSettingsNode );
    ImGuiSetNextWindowStretch();

    if( !ImGuiBegin( "System Window" ) )
      return;


    if( ImGuiCollapsingHeader( "Select System" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( const SystemRegistryEntry& systemRegistryEntry : SystemRegistryIterator() )
      {
        if( ImGuiButton( systemRegistryEntry.mName ) )
        {
          sSystemRegistryEntry = &systemRegistryEntry;
          mSettingsNode.GetChild( nSysPath ).SetValue( systemRegistryEntry.mName );
        }

        if( sSystemRegistryEntry == &systemRegistryEntry )
        {
          ImGuiSameLine();
          ImGuiText( "<-- currently selected" );
        }
      }
    }

    const ShortFixedString headerStr{
      ShortFixedString::Concat( sSystemRegistryEntry->mName, " Debug" ) };

    if( sSystemRegistryEntry &&
        sSystemRegistryEntry->mDebugImGui &&
        ImGuiCollapsingHeader( headerStr ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      System* system { gCreation.mWorld->GetSystem( sSystemRegistryEntry ) };
      sSystemRegistryEntry->mDebugImGui( system );
    }

    if( ImGuiButton( "Close Window" ) )
      sShowWindow = false;

    ImGuiEnd();
  }
}

