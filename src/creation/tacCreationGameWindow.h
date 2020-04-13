#pragma once

#include "src/common/tacString.h"
#include "src/common/math/tacVector3.h"
#include "src/common/tacDesktopWindow.h"

namespace Tac
{


struct Entity;
struct Model;
struct Errors;
struct DesktopWindow;
struct Creation;
struct UIRoot;
struct Debug3DDrawData;
struct UI2DDrawData;
struct Soul;
struct Shell;
struct RenderView;
struct Shader;
struct VertexFormat;
struct CBuffer;
struct DepthState;
struct BlendState;
struct RasterizerState;
struct SamplerState;
struct Mesh;
struct DefaultCBufferPerObject;
struct GamePresentation;
struct SkyboxPresentation;

struct CreationGameWindow
{
  ~CreationGameWindow();
  void Init( Errors& errors );
  void Update( Errors& errors );
  void RenderGameWorldToGameWindow();
  void MousePickingInit();
  void MousePickingEntity( const Entity* entity, bool* hit, float* dist );
  void MousePickingAll();
  void AddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& cbuf );
  void ComputeArrowLen();
  void CameraControls();
  void CreateGraphicsObjects( Errors& errors );
  void DrawPlaybackOverlay( Errors& errors );
  void PlayGame( Errors& errors );

  
  DesktopWindowState mDesktopWindowState;
  DesktopWindow* mDesktopWindow = nullptr;
  UIRoot* mUIRoot = nullptr;
  UI2DDrawData* mUI2DDrawData = nullptr;
  Soul* mSoul = nullptr;
  Creation* mCreation = nullptr;

  // Renderer resources
  Shader* m3DShader = nullptr;
  VertexFormat* m3DVertexFormat = nullptr;
  CBuffer* mPerFrame = nullptr;
  CBuffer* mPerObj = nullptr;
  DepthState* mDepthState = nullptr;
  BlendState* mBlendState = nullptr;
  RasterizerState* mRasterizerState = nullptr;
  SamplerState* mSamplerState = nullptr;

  Debug3DDrawData* mDebug3DDrawData = nullptr;

  GamePresentation* mGamePresentation = nullptr;
  SkyboxPresentation* mSkyboxPresentation = nullptr;

  Mesh* mArrow = nullptr;
  Mesh* mCenteredUnitCube = nullptr;
  v3 worldSpaceMouseDir = {};
  float mArrowLen = 0;

  String mStatusMessage;
  double mStatusMessageEndTime = 0;
};


const String gGameWindowName = "VirtualGamePlayer";
}
