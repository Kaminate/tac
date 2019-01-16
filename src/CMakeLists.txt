include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


# This needs to be first in order to define ${TacCommon}?
add_subdirectory(common)

add_subdirectory(space)
add_subdirectory(shell)
add_subdirectory(creation)

# add_dependencies(Shell Ghost)
