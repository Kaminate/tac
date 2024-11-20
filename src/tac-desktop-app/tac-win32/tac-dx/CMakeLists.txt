add_library( TacDX )
set_target_properties( TacDX PROPERTIES FOLDER "Engine/Desktop App" )
target_include_directories( TacDX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

add_subdirectory( dxc )
add_subdirectory( dxgi )
add_subdirectory( dx12 )
add_subdirectory( pix )
add_subdirectory( hlsl )


# Download pix runtime library from nuget
if( TRUE )
  include( ${CMAKE_SOURCE_DIR}/cmake/pix.cmake )
  add_pix_target()
  target_link_libraries( TacDX PIX )
  add_custom_command( TARGET TacDX
                      PRE_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                              ${PIX_DLL_PATH}
                              $<TARGET_FILE_DIR:TacDX> )
  target_compile_definitions( TacDX PUBLIC TAC_PIX_NUGET )
endif()
