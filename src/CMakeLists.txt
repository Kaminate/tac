add_subdirectory(tac-engine-core)

# make this second, as space uses common,
# and creation/game uses space?
# and shell uses space
add_subdirectory(space)

add_subdirectory(shell)
add_subdirectory(level_editor)
add_subdirectory(level_editor_2)
add_subdirectory(game)
add_subdirectory(game-examples)
add_subdirectory(game-weekendtrace)
add_subdirectory(game-pbrt)

