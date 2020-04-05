#pragma once

namespace Tac
{

  struct BlendState;
  struct CBuffer;
  struct Camera;
  struct DefaultCBufferPerObject;
  struct DepthState;
  struct DesktopWindow;
  struct Entity;
  struct Mesh;
  struct ModelAssetManager;
  struct RasterizerState;
  struct Renderer;
  struct SamplerState;
  struct Shader;
  struct SkyboxPresentation;
  struct VertexFormat;
  struct World;
  struct Debug3DCommonData;


  struct GamePresentation
  {
    ~GamePresentation();
    void CreateGraphicsObjects( Errors& errors );
    void RenderGameWorldToDesktopView();
    void RenderGameWorldAddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& cbuf );

    Camera* mCamera = nullptr;
    DesktopWindow* mDesktopWindow = nullptr;
    World* mWorld = nullptr;

    SkyboxPresentation* mSkyboxPresentation = nullptr;

    // Renderer resources
    Shader* m3DShader = nullptr;
    Shader* mTerrainShader = nullptr;
    VertexFormat* m3DVertexFormat = nullptr;
    VertexFormat* mTerrainVertexFormat = nullptr;
    CBuffer* mPerFrame = nullptr;
    CBuffer* mPerObj = nullptr;
    DepthState* mDepthState = nullptr;
    BlendState* mBlendState = nullptr;
    RasterizerState* mRasterizerState = nullptr;
    SamplerState* mSamplerState = nullptr;

  private:
    void Create3DShader( Errors& errors );
    void CreateTerrainShader( Errors& errors );
    void Create3DVertexFormat( Errors& errors );
    void CreateTerrainVertexFormat( Errors& errors );
    void CreatePerFrame( Errors& errors );
    void CreatePerObj( Errors& errors );
    void CreateDepthState( Errors& errors );
    void CreateBlendState( Errors& errors );
    void CreateRasterizerState( Errors& errors );
    void CreateSamplerState( Errors& errors );
  };
}

