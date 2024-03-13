target_sources( TacEngineCore PRIVATE
  tac_mesh.cpp
  tac_mesh.h
  tac_model_asset_manager.cpp
  tac_model_asset_manager.h
  tac_model_asset_manager_backend.cpp
  tac_model_asset_manager_backend.h
  tac_texture_asset_manager.cpp
  tac_texture_asset_manager.h
)

add_subdirectory( gltf )
add_subdirectory( obj )

