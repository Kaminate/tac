add_library( TacECS
  "tac_space.cpp"
  "tac_space.h"
  "tac_space_types.cpp"
  "tac_space_types.h"
)

set_target_properties( TacECS PROPERTIES FOLDER "Engine" )

add_subdirectory( component )
add_subdirectory( entity )
add_subdirectory( system )
add_subdirectory( ghost )
add_subdirectory( graphics )
add_subdirectory( net )
add_subdirectory( physics )
add_subdirectory( player )
add_subdirectory( presentation )
add_subdirectory( script )
add_subdirectory( scripts )
add_subdirectory( terrain )
add_subdirectory( world )

# target_precompile_headers( TacECS PRIVATE "${CMAKE_SOURCE_DIR}/tac-std-lib/tac_precompiled_header.h")

target_include_directories( TacECS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../")

target_link_libraries( TacECS TacEngineCore )



