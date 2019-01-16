project(Ghost CXX)
set(TacTarget "Ghost")

add_library(${TacTarget} MODULE
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
${TacCommon})

include_directories("${CMAKE_SOURCE_DIR}/src")

set_target_properties(${TacTarget} PROPERTIES LINKER_LANGUAGE CXX)



