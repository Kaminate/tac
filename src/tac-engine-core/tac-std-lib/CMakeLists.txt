add_library( TacStdLib )
set_target_properties( TacStdLib PROPERTIES FOLDER "Engine/Core" )

target_sources( TacStdLib PRIVATE
  tac_std_lib.natvis
  tac_std_lib.natstepfilter
  # tac_std_lib.h
  tac_ints.h
  tac_type_traits.h
  tac_std_lib_pch.h
)

add_subdirectory( algorithm )
add_subdirectory( containers )
add_subdirectory( dataprocess )
add_subdirectory( error )
add_subdirectory( filesystem )
add_subdirectory( math )
add_subdirectory( memory )
add_subdirectory( meta )
add_subdirectory( mutex )
add_subdirectory( os )
add_subdirectory( preprocess )
add_subdirectory( string )

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

target_include_directories( TacStdLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

# This does weird things where sometimes after rebuilding ( without cleaning ) the program will immediately exit
# without hitting main
#
# Given the list of input files (just tac_precompiled_header.h atm),
# cmake precompiles them into cmake_pch.cxx and force includes them (/FI MSVC) into each of the target (TacStdLib)
# source files.
target_precompile_headers( TacStdLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tac_std_lib_pch.h )

