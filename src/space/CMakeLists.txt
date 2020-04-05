project(Ghost CXX)
set(Target "Ghost")
set( TAC_LIBRARIES CommonLibrary  )
#                        | ( commented out )
#                        |
#                        | | MODULE is for DLLs
#                        v v


# ${CMAKE_CURRENT_SOURCE_DIR}/myfile.h

add_library(${Target} # MODULE
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tacGamePresentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tacGamePresentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tacSkyboxPresentation.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/presentation/tacSkyboxPresentation.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacClient.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacClient.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacComponent.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacComponent.h"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tacCollider.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tacCollider.h"
"${CMAKE_CURRENT_SOURCE_DIR}/collider/tacColliderDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacEntity.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacEntity.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacGjk.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacGjk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacGhost.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacGhost.h"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacGraphicsDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacGraphics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacGraphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tacModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tacModel.h"
"${CMAKE_CURRENT_SOURCE_DIR}/model/tacModelDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacLight.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacLight.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacPlayer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacPlayer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tacMetaPhysics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tacPhysicsDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tacPhysics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/physics/tacPhysics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacScript.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacScript.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacScriptGameClient.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacScriptGameClient.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSpace.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSpace.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSpacetypes.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSystem.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSystem.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacServer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacServer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSpacenet.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacSpacenet.h"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tacTerrain.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tacTerrain.h"
"${CMAKE_CURRENT_SOURCE_DIR}/terrain/tacTerrainDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacWorld.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/tacWorld.h"
)
#${Common})
include_directories("${CMAKE_SOURCE_DIR}/src")
set_target_properties(${Target} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries( ${Target} ${TAC_LIBRARIES} )



