#include "tac_level_editor_system_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
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
  static const SystemInfo*          sSystemInfo{};
  static const char*                nSysPath{ "SystemWindow.nSys" };
  static bool                       sInitialized;
  static const StringView           sDefaultSystem{ "Graphics" };

  static void Initialize()
  {
    SettingsNode mSettingsNode{ Shell::sShellSettings };
    const String systemName{ mSettingsNode.GetChild( nSysPath ).GetValueWithFallback( "" ) };
    for( const SystemInfo& entry : SystemInfo::Iterate() )
      if( entry.mName == systemName.c_str() )
        sSystemInfo = &entry;
  }

  // -----------------------------------------------------------------------------------------------

  bool CreationSystemWindow::sShowWindow{};
  void CreationSystemWindow::Update()
  {
    if( !sShowWindow )
      return;

    Initialize();
    ImGuiSetNextWindowStretch();

    if( !ImGuiBegin( "System Window" ) )
      return;


    if( ImGuiCollapsingHeader( "Select System", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( const SystemInfo& systemRegistryEntry : SystemInfo::Iterate() )
      {
        if( ImGuiButton( systemRegistryEntry.mName )
            || ( !sSystemInfo && ( StringView )systemRegistryEntry.mName == sDefaultSystem ) )
        {
          sSystemInfo = &systemRegistryEntry;
          SettingsNode settingsNode{ Shell::sShellSettings };
          settingsNode.GetChild( nSysPath ).SetValue( systemRegistryEntry.mName );
        }

        if( sSystemInfo == &systemRegistryEntry )
        {
          ImGuiSameLine();
          ImGuiText( "<-- currently selected" );
        }
      }
    }

    if( sSystemInfo && sSystemInfo->mDebugImGui )
    {
      if( const ShortFixedString headerStr{
        ShortFixedString::Concat( sSystemInfo->mName, " Debug" ) };
        ImGuiCollapsingHeader( headerStr, ImGuiNodeFlags_DefaultOpen ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        Creation::Data* data{ Creation::GetData() };
        sSystemInfo->mDebugImGui( data->mWorld.GetSystem( sSystemInfo ) );
      }
    }

    if( ImGuiButton( "Close Window" ) )
      sShowWindow = false;

    ImGuiEnd();
  }
}

