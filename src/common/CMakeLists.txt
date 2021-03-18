
# TODO:
#   since we're compiling this into a static library now,
#   we no longer need the Common global variable

set(Common 
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacMesh.cpp
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacMesh.h
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacModelLoadSynchronous.cpp
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacModelLoadSynchronous.h
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacModelAssetManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacModelAssetManager.h
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacTextureAssetManager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/assetmanagers/tacTextureAssetManager.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacArray.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacColorUtil.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacColorUtil.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacFont.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacFont.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/imgui/tacImGui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/imgui/tacImGui.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/imgui/tacImGuiState.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/imgui/tacImGuiState.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacTextEdit.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacTextEdit.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacUI2D.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacUI2D.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacDebug3D.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacDebug3D.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRendererBackend.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRendererBackend.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRenderer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRenderer.h
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRendererUtil.cpp
${CMAKE_CURRENT_SOURCE_DIR}/graphics/tacRendererUtil.h
${CMAKE_CURRENT_SOURCE_DIR}/tacAlgorithm.h
${CMAKE_CURRENT_SOURCE_DIR}/tacCamera.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacCamera.h
${CMAKE_CURRENT_SOURCE_DIR}/tacCommon.natvis
${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindow.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacDesktopWindow.h
${CMAKE_CURRENT_SOURCE_DIR}/tacErrorHandling.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacErrorHandling.h
${CMAKE_CURRENT_SOURCE_DIR}/tacEvent.h
${CMAKE_CURRENT_SOURCE_DIR}/tacFrameMemory.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacIDCollection.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacIDCollection.h
${CMAKE_CURRENT_SOURCE_DIR}/tacJson.cpp
${CMAKE_CURRENT_SOURCE_DIR}/taccontrollerinput.cpp
${CMAKE_CURRENT_SOURCE_DIR}/taccontrollerinput.h
${CMAKE_CURRENT_SOURCE_DIR}/tackeyboardinput.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tackeyboardinput.h
${CMAKE_CURRENT_SOURCE_DIR}/tacFrameMemory.h
${CMAKE_CURRENT_SOURCE_DIR}/tacJson.h
${CMAKE_CURRENT_SOURCE_DIR}/tacLocalization.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacLocalization.h
${CMAKE_CURRENT_SOURCE_DIR}/tacLog.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacLog.h
${CMAKE_CURRENT_SOURCE_DIR}/tacTemporaryMemory.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacTemporaryMemory.h
${CMAKE_CURRENT_SOURCE_DIR}/tacTextParser.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacTextParser.h
${CMAKE_CURRENT_SOURCE_DIR}/tacMemory.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacMemory.h
${CMAKE_CURRENT_SOURCE_DIR}/profile/tacProfile.cpp
${CMAKE_CURRENT_SOURCE_DIR}/profile/tacProfile.h
${CMAKE_CURRENT_SOURCE_DIR}/profile/tacProfileImGui.cpp
${CMAKE_CURRENT_SOURCE_DIR}/profile/tacProfileImGui.h
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
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacFixedVector.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacFrameVector.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacRingBuffer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacRingBuffer.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacRingVector.h
${CMAKE_CURRENT_SOURCE_DIR}/tacSerialization.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacSerialization.h
${CMAKE_CURRENT_SOURCE_DIR}/tacShellTimer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacShellTimer.h
${CMAKE_CURRENT_SOURCE_DIR}/tacShell.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacShell.h
${CMAKE_CURRENT_SOURCE_DIR}/tacSettings.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacSettings.h
${CMAKE_CURRENT_SOURCE_DIR}/tacHash.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacHash.h
${CMAKE_CURRENT_SOURCE_DIR}/string/tacString.cpp
${CMAKE_CURRENT_SOURCE_DIR}/string/tacString.h
${CMAKE_CURRENT_SOURCE_DIR}/string/tacStringIdentifier.cpp
${CMAKE_CURRENT_SOURCE_DIR}/string/tacStringIdentifier.h
${CMAKE_CURRENT_SOURCE_DIR}/tacTime.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacTime.h
${CMAKE_CURRENT_SOURCE_DIR}/tacUtility.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacUtility.h
${CMAKE_CURRENT_SOURCE_DIR}/containers/tacVector.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector2.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector2.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector3.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector3.h
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector4.cpp
${CMAKE_CURRENT_SOURCE_DIR}/math/tacVector4.h
${CMAKE_CURRENT_SOURCE_DIR}/tacJobQueue.cpp
${CMAKE_CURRENT_SOURCE_DIR}/tacJobQueue.h
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cgltf.h
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_image.h
${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_truetype.h
CACHE INTERNAL ""
)

IF (WIN32)
  # Program Database for Edit & Continue
  add_definitions(/ZI)
ENDIF()

# Create a library named CommonLibrary
add_library( "CommonLibrary" ${Common} )
target_precompile_headers( "CommonLibrary" 

# PUBLIC 
PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/tacPrecompiledHeader.h
)

