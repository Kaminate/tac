
# Create a library named TacEngineCore
add_library( TacEngineCore )

target_sources( TacEngineCore PRIVATE
  tac_engine_core.natvis
  tac_engine_core.natstepfilter
  tac_engine_core.h
  tac_engine_core_pch.h
)

# Add the shader files to the Visual Studio Solution Explorer
if( WIN32 )

  # Add to the solution explorer
  target_sources( TacEngineCore PRIVATE ${TAC_SHADERS} )

  # Group these target_sources under a "shaders" filter
  source_group( "shaders" FILES ${TAC_SHADERS} )

  # Disable visual studio auto-shader build
  #
  #   Visual studio adds .hlsl files as a <FXCompile> item in the .vcxproj.
  #   When you try to build your project, the fxc/dxc shader compiler tries (and fails) to compile
  #   these <FXCompile> objects, which outputs errors and stops the whole build.
  #   Here we exclude the shaders from the build so that we can call dxc when we want later.
  set_source_files_properties( SOURCE ${TAC_SHADERS}
                               PROPERTIES
                               VS_SETTINGS "ExcludedFromBuild=true" )
endif()


add_subdirectory( assetmanagers )
add_subdirectory( framememory )
add_subdirectory( graphics )
add_subdirectory( i18n )
add_subdirectory( input )
add_subdirectory( net )
add_subdirectory( profile )
add_subdirectory( settings )
add_subdirectory( shell )
add_subdirectory( system )
add_subdirectory( thirdparty )

add_subdirectory(tac-std-lib)
add_subdirectory(tac-rhi)

target_link_libraries( TacStdLib TacRHI )

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

target_include_directories( TacEngineCore PUBLIC ${CMAKE_SOURCE_DIR})

# This does weird things where sometimes after rebuilding ( without cleaning ) the program will immediately exit
# without hitting main
#
# Given the list of input files (just tac_precompiled_header.h atm),
# cmake precompiles them into cmake_pch.cxx and force includes them (/FI MSVC) into each of the target (TacEngineCore)
# source files.
target_precompile_headers( TacEngineCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tac_precompiled_header.h)

