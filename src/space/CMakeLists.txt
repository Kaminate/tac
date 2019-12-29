project(Ghost CXX)
set(TacTarget "Ghost")
set( TAC_LIBRARIES TacCommonLibrary  )
#                        | ( commented out )
#                        |
#                        | | MODULE is for DLLs
#                        v v
add_library(${TacTarget} # MODULE
"presentation/tacGamePresentation.cpp"
"presentation/tacGamePresentation.h"
"presentation/tacSkyboxPresentation.cpp"
"presentation/tacSkyboxPresentation.h"
"tacclient.cpp"
"tacclient.h"
"taccomponent.cpp"
"taccomponent.h"
"collider/taccollider.cpp"
"collider/taccollider.h"
"collider/taccolliderdebug.cpp"
"tacentity.cpp"
"tacentity.h"
"tacgjk.cpp"
"tacgjk.h"
"tacGhost.cpp"
"tacGhost.h"
"graphics/tacgraphicsdebug.cpp"
"graphics/tacgraphics.cpp"
"graphics/tacgraphics.h"
"model/tacmodel.cpp"
"model/tacmodel.h"
"model/tacmodeldebug.cpp"
"taclight.cpp"
"taclight.h"
"tacplayer.cpp"
"tacplayer.h"
"physics/tacmetaphysics.cpp"
"physics/tacphysicsdebug.cpp"
"physics/tacphysics.cpp"
"physics/tacphysics.h"
"tacscript.cpp"
"tacscript.h"
"tacscriptgameclient.cpp"
"tacscriptgameclient.h"
"tacspace.cpp"
"tacspace.h"
"tacspacetypes.h"
"tacsystem.cpp"
"tacsystem.h"
"tacserver.cpp"
"tacserver.h"
"tacspacenet.cpp"
"tacspacenet.h"
"terrain/tacterrain.cpp"
"terrain/tacterrain.h"
"terrain/tacterraindebug.cpp"
"tacworld.cpp"
"tacworld.h"
)
#${TacCommon})
include_directories("${CMAKE_SOURCE_DIR}/src")
set_target_properties(${TacTarget} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries( ${TacTarget} ${TAC_LIBRARIES} )



