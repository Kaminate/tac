
#list( APPEND TAC_SOURCES
#    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_meta.cpp"
##    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_meta.h"
#)
#
#set( TAC_SOURCES ${TAC_SOURCES} CACHE INTERNAL "" FORCE )

target_sources( TacExamples PRIVATE
"tac_example_meta.cpp"
"tac_example_meta.h"
)

