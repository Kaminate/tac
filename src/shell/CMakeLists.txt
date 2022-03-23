add_subdirectory(windows)
add_subdirectory(opengl)
add_subdirectory(sdl)
add_subdirectory(vulkan)

set("TAC_DESKTOP_APP_FILES"
"${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopApp.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopApp.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindowGraphics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindowGraphics.h"
CACHE INTERNAL "")
