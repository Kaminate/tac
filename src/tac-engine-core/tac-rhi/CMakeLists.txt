

# Create a library named TacEngineCore
add_library( TacRHI )
set_target_properties( TacRHI PROPERTIES FOLDER "Engine/Core" )

if( MSVC )
  target_sources( TacRHI PRIVATE
    tac_rhi.natvis
    tac_rhi.natstepfilter
  )
endif()

add_subdirectory( identifier )
# add_subdirectory( render )
add_subdirectory( render3 )
# add_subdirectory( renderer )

target_link_libraries( TacRHI TacStdLib )

# -------------------------------------------
# Create visual studio project filters
if( MSVC )
  file( GLOB_RECURSE source_list RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp *.h *.natvis)
  foreach(source IN LISTS source_list)
      cmake_path( NATIVE_PATH source native_source )
      cmake_path( GET native_source PARENT_PATH native_parent )
      source_group("${native_parent}" FILES "${source}")
  endforeach()  
endif()
# -------------------------------------------

target_include_directories( TacRHI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )


