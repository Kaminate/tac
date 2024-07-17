add_library( TacDX )
target_include_directories( TacDX PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

add_subdirectory( dxc )
add_subdirectory( dxgi )
add_subdirectory( dx12 )
add_subdirectory( pix )
add_subdirectory( hlsl )

