
# TODO:
#   since we're compiling this into a static library now,
#   we no longer need the TacCommon global variable

set(TacCommon 
${CMAKE_CURRENT_SOURCE_DIR}/imconfig.cpp
${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h

${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/imgui.h
${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
${CMAKE_CURRENT_SOURCE_DIR}/imgui_internal.h
${CMAKE_CURRENT_SOURCE_DIR}/tacAlgorithm.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacArray.h
${CMAKE_CURRENT_SOURCE_DIR}/tacColorUtil.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacColorUtil.h
${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindow.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindow.h
${CMAKE_CURRENT_SOURCE_DIR}/tacErrorHandling.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacErrorHandling.h
${CMAKE_CURRENT_SOURCE_DIR}/tacEvent.h
${CMAKE_CURRENT_SOURCE_DIR}/tacFont.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacFont.h
${CMAKE_CURRENT_SOURCE_DIR}/taccontrollerinput.cpp
${CMAKE_CURRENT_SOURCE_DIR}/taccontrollerinput.h
${CMAKE_CURRENT_SOURCE_DIR}/tackeyboardinput.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tackeyboardinput.h
${CMAKE_CURRENT_SOURCE_DIR}/tacJson.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacJson.h
${CMAKE_CURRENT_SOURCE_DIR}/tacLocalization.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacLocalization.h
${CMAKE_CURRENT_SOURCE_DIR}/tacLog.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacLog.h
${CMAKE_CURRENT_SOURCE_DIR}/tacMemory.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacMemory.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMath.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMath.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMatrix3.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMatrix3.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMatrix4.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacMatrix4.h
${CMAKE_CURRENT_SOURCE_DIR}/tacmeta.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacmeta.h
${CMAKE_CURRENT_SOURCE_DIR}/tacNet.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacNet.h
${CMAKE_CURRENT_SOURCE_DIR}/tacOS.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacOS.h
${CMAKE_CURRENT_SOURCE_DIR}/tacPreprocessor.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacPreprocessor.h
${CMAKE_CURRENT_SOURCE_DIR}/tacRenderer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacRenderer.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacRingVector.h
${CMAKE_CURRENT_SOURCE_DIR}/tacSerialization.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacSerialization.h
${CMAKE_CURRENT_SOURCE_DIR}/tacShell.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacShell.h
${CMAKE_CURRENT_SOURCE_DIR}/tacSettings.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacSettings.h
${CMAKE_CURRENT_SOURCE_DIR}/tacString.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacString.h
${CMAKE_CURRENT_SOURCE_DIR}/tacSpan.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacSpan.h
${CMAKE_CURRENT_SOURCE_DIR}/tacTime.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacTime.h
${CMAKE_CURRENT_SOURCE_DIR}/tacTextureAssetManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacTextureAssetManager.h
${CMAKE_CURRENT_SOURCE_DIR}/tacUI.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacUI.h
${CMAKE_CURRENT_SOURCE_DIR}/tacUI2D.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacUI2D.h
${CMAKE_CURRENT_SOURCE_DIR}/tacUtility.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacUtility.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacVector.cpp
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacVector.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector2.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector3.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector3.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector4.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector4.h
${CMAKE_CURRENT_SOURCE_DIR}/tacJobQueue.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacJobQueue.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_rect_pack.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_textedit.h
${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype.h
CACHE INTERNAL ""
)

# Create a library named TacCommonLibrary
add_library( "TacCommonLibrary" ${TacCommon} )

