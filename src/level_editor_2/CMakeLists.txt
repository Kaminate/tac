# only build editor for desktop (not console)
if(NOT (WIN32) )
  return()
endif()


add_executable( LevelEditor2 WIN32 )

target_link_libraries( LevelEditor2 ) 

target_link_libraries( LevelEditor2 
  TacWin32
  TacWin32Main
  TacEngineCore
  TacDesktopApp
  TacGhost )

target_sources( LevelEditor2 PRIVATE 
#  "tac_entity_selection.cpp"
#  "tac_entity_selection.h"
#  "tac_level_editor_window_manager.cpp"
#  "tac_level_editor_window_manager.h"
  "tac_level_editor.cpp"
  "tac_level_editor.h"
#  "tac_level_editor_asset_view.cpp"
#  "tac_level_editor_asset_view.h"
#  "tac_level_editor_prefab.cpp"
#  "tac_level_editor_prefab.h"
#  "tac_level_editor_game_object_menu_window.cpp"
#  "tac_level_editor_game_object_menu_window.h"
#  "tac_level_editor_game_window.cpp"
#  "tac_level_editor_game_window.h"
#  "tac_level_editor_main_window.cpp"
#  "tac_level_editor_main_window.h"
#  "tac_level_editor_profile_window.cpp"
#  "tac_level_editor_profile_window.h"
#  "tac_level_editor_property_window.cpp"
#  "tac_level_editor_property_window.h"
#  "tac_level_editor_system_window.cpp"
#  "tac_level_editor_system_window.h"
)

target_precompile_headers( LevelEditor2 PRIVATE 
  "${CMAKE_SOURCE_DIR}/tac-std-lib/tac_precompiled_header.h"
)

target_include_directories( LevelEditor2 PUBLIC 
#  ${TAC_INCLUDE_DIRECTORIES} 
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src" )


