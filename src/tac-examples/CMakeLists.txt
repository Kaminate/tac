include( CMakePrintHelpers )

set( TAC_DEBUGGING FALSE )

add_executable( TacExamples WIN32 )

target_sources( TacExamples PRIVATE
  "tac_examples.cpp"
  "tac_examples.h"
  "tac_examples_registry.cpp"
  "tac_examples_registry.h"
  "tac_examples_state_machine.cpp"
  "tac_examples_state_machine.h"
  "tac_render_tutorial_examples.cpp"
  "tac_render_tutorial_examples.h"
)

target_link_libraries( TacExamples PUBLIC TacECS )

add_subdirectory( dx12 )
add_subdirectory( render_tutorial )
add_subdirectory( fluid )
add_subdirectory( meta )
add_subdirectory( phy_sim )
add_subdirectory( vk )
add_subdirectory( text )
add_subdirectory( imgui )
add_subdirectory( radiosity_math )

target_include_directories( TacExamples PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

set_target_properties( TacExamples PROPERTIES FOLDER "Programs" )

target_link_libraries( TacExamples
  PUBLIC
  TacEngineCore
  TacDesktopApp
  TacECS
  TacWin32
  TacWin32Main

  microtex # for radiosity_math
)

if( TARGET TacVkRenderer )
  target_link_libraries( TacExamples PUBLIC TacVkRenderer )
  target_link_libraries( TacExamples PUBLIC TacWinVk )
endif()

