target_sources( TacEngineCore PRIVATE
  tac_key.cpp
  tac_key.h
  tac_app_keyboard_api.h
  tac_sys_keyboard_api.h
  tac_sim_keyboard_api.h
  tac_keyboard_backend.cpp
  tac_keyboard_backend.h
)

add_subdirectory( controller )
add_subdirectory( mouseedge )


