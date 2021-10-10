include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


# This needs to be first in order to define ${TacCommon}?
add_subdirectory(common)
add_subdirectory(tac-core)

# make this second, as space uses common,
# and creation/game uses space?
add_subdirectory(space)

add_subdirectory(shell)
add_subdirectory(creation)
add_subdirectory(game)

#                  | target, target-deps
#                  v 
# add_dependencies(Shell Ghost)
