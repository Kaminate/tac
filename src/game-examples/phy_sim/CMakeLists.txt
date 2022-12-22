
list( APPEND TAC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_1_force.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_1_force.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_2_integration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_2_integration.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_3_torque.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_3_torque.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_4_tank.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_4_tank.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_5_lincollision.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_5_lincollision.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_6_rotcollision.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_6_rotcollision.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_7_friction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tac_example_phys_sim_7_friction.h"
)

set( TAC_SOURCES ${TAC_SOURCES} CACHE INTERNAL "" FORCE )
