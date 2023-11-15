#list( APPEND TAC_SOURCES
#  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_d3d11_1_tri.h"
#  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_d3d11_2_tri_textured.h"
#  "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_d3d11_3_cube_spin.h"
#)
# set( TAC_SOURCES ${TAC_SOURCES} CACHE INTERNAL "" FORCE )

target_sources( TacExamples PRIVATE
  "tac_example_d3d11_1_tri.h"
  "tac_example_d3d11_2_tri_textured.h"
  "tac_example_d3d11_3_cube_spin.h"
)



