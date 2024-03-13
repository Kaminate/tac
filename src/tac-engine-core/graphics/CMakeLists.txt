target_sources( TacEngineCore PRIVATE
  tac_renderer_util.cpp
  tac_renderer_util.h
)

add_subdirectory( camera )
add_subdirectory( color )
add_subdirectory( debug )
add_subdirectory( ui )
