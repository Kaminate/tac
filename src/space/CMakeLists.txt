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
"taccollider.cpp"
"taccollider.h"
"tacentity.cpp"
"tacentity.h"
"tacgjk.cpp"
"tacgjk.h"
"tacGhost.cpp"
"tacGhost.h"
"tacgraphics.cpp"
"tacgraphics.h"
"tacmodel.cpp"
"tacmodel.h"
"tacmetaphysics.cpp"
"tacplayer.cpp"
"tacplayer.h"
"tacphysics.cpp"
"tacphysics.h"
"tacscript.cpp"
"tacscript.h"
"tacscriptgameclient.cpp"
"tacscriptgameclient.h"
"tacspacetypes.h"
"tacsystem.cpp"
"tacsystem.h"
"tacserver.cpp"
"tacserver.h"
"tacspacenet.cpp"
"tacspacenet.h"
"tacworld.cpp"
"tacworld.h"
)
#${TacCommon})
include_directories("${CMAKE_SOURCE_DIR}/src")
set_target_properties(${TacTarget} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries( ${TacTarget} ${TAC_LIBRARIES} )



