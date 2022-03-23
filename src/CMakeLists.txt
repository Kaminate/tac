include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


# This needs to be first in order to define ${TacCommon}?
add_subdirectory(common)

# make this second, as space uses common,
# and creation/game uses space?
add_subdirectory(space)

add_subdirectory(shell)
add_subdirectory(creation)
add_subdirectory(game)
add_subdirectory(game-examples)
add_subdirectory(game-weekendtrace)

#                  | target, target-deps
#                  v 
# add_dependencies(Shell Ghost)
