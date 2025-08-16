add_library(microtex STATIC "")
set_target_properties( microtex PROPERTIES FOLDER "Third-Party" )

if (MSVC)
    target_compile_options(microtex PRIVATE "/utf-8")
    # target_compile_features(microtex PUBLIC cxx_std_17)
    target_compile_definitions(microtex PRIVATE -DMICROTEX_LIBRARY)
    # target_compile_definitions(microtex PUBLIC -D_HAS_STD_BYTE=1)
endif ()

set_target_properties( microtex PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_subdirectory(atom)
add_subdirectory(box)
add_subdirectory(core)
add_subdirectory(env)
add_subdirectory(graphic)
add_subdirectory(macro)
add_subdirectory(otf)
add_subdirectory(render)
add_subdirectory(unimath)
add_subdirectory(utils)
target_sources(microtex PRIVATE microtex.cpp PUBLIC microtex.h)


target_include_directories( microtex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )

# target_include_directories(microtex PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
# target_include_directories( microtex PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/microtex>)

# 0: use path and typeface both
# 1: use path only
# 2: use typeface only
target_compile_definitions(microtex PRIVATE -DGLYPH_RENDER_TYPE=0)

# configure_file(microtexconfig.h.in microtexconfig.h @ONLY)

