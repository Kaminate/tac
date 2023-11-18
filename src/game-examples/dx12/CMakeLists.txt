# include( CMakePrintHelpers )

if( NOT WIN32 )
  return()
endif()

project( TacDX12HelloWorld )

add_executable( TacDX12HelloWindow WIN32 )

target_sources( TacDX12HelloWindow PRIVATE
  "tac_example_dx12_1_window.cpp"
  "tac_example_dx12_1_window.h"
  ${TAC_WIN_MAIN_FILES}
)

target_link_libraries( TacDX12HelloWindow
  TacCommon
  TacGhost
  $<TARGET_OBJECTS:TacDesktopApp>
  TacWin
  #TacExamples
)

set_target_properties( TacDX12HelloWindow PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  LINKER_LANGUAGE CXX )

# target_precompile_headers( TacDX12HelloWindow PRIVATE "${CMAKE_SOURCE_DIR}/src/common/tac_precompiled_header.h" )

target_compile_options( TacDX12HelloWindow PUBLIC /ZI ) # Program Database for Edit & Continue

