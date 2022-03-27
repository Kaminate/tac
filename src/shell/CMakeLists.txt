add_subdirectory(windows)
add_subdirectory(opengl)
add_subdirectory(sdl)
add_subdirectory(vulkan)

set("TAC_DESKTOP_APP_FILES"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_app.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_app.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_window_graphics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_window_graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_window_settings_tracker.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tac_desktop_window_settings_tracker.h"

CACHE INTERNAL "")
