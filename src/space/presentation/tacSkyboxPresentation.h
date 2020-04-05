#pragma once

namespace Tac
{


struct BlendState;
struct CBuffer;
struct Camera;
struct DepthState;
struct DesktopWindow;
struct Errors;
struct ModelAssetManager;
struct RasterizerState;
struct Renderer;
struct SamplerState;
struct Shader;
struct String;
struct TextureAssetManager;
struct VertexFormat;

struct SkyboxPresentation
{
  ~SkyboxPresentation();
  void RenderSkybox( const String& skyboxDir );
  void Init( Errors& errors );

  Camera* mCamera = nullptr;
  DesktopWindow* mDesktopWindow = nullptr;

  VertexFormat* mVertexFormat = nullptr;
  Shader* mShader = nullptr;
  CBuffer* mPerFrame = nullptr;
  BlendState* mBlendState = nullptr;
  DepthState* mDepthState  = nullptr;
  RasterizerState* mRasterizerState = nullptr;
  SamplerState* mSamplerState = nullptr;
};
}
