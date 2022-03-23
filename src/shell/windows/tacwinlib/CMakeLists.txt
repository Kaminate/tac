if( WIN32 )

set(Target "TacWinLib")

project( ${Target} CXX )
set( TAC_LIBRARIES CommonLibrary  )

set("TAC_WIN_LIB_FILES"
"${CMAKE_CURRENT_SOURCE_DIR}/net/tacNetWinsock.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/net/tacNetWinsock.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacDXGI.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacDXGI.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX11.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX11.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX11ShaderPreprocess.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX12.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectX12.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererDirectXPIX.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererOpenGL4Win32.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderer/tacRendererVulkanWin32.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/desktopwindow/tacWin32DesktopWindowManager.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/desktopwindow/tacWin32DesktopWindowManager.h"
"${CMAKE_CURRENT_SOURCE_DIR}/input/tacWin32MouseEdge.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/input/tacWin32MouseEdge.h"
"${CMAKE_CURRENT_SOURCE_DIR}/input/tacXInput.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/input/tacXInput.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacWin32.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacWin32.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacWin32OS.cpp"
CACHE INTERNAL ""
)
add_library(${Target} ${TAC_WIN_LIB_FILES})

set_target_properties(${Target} PROPERTIES LINKER_LANGUAGE CXX)
add_definitions(/ZI) # produce separate pdb file ( implies /debug )
target_link_libraries( ${Target} ${TAC_LIBRARIES} )

endif() # if WIN32
