#include "tac_render_material.h" // self-inc

#include "tac_render_material_cache.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  static Render::DepthState      GetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static Render::BlendState      GetBlendState()
  {
    const Render::BlendState state
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::Zero },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add },
    };
    return state;
  }

  static Render::RasterizerState GetRasterizerState()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::Back },
      .mFrontCounterClockwise { true },
      .mMultisample           {},
    };
  }

  static Render::ProgramHandle   Create3DShader( const ShaderGraph& shaderGraph, Errors& errors )
  {
    const Vector< String > inputs
    {
      shaderGraph.mMaterialShader,
      "Material.hlsl",
    };

    const Render::ProgramParams programParams
    {
      .mInputs     { inputs },
      .mStackFrame { TAC_STACK_FRAME },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateProgram( programParams, errors );
  }

  static Render::PipelineHandle  CreatePipeline( Render::ProgramHandle program, Errors& errors )
  {
    const Render::BlendState blendState{ GetBlendState() };
    const Render::DepthState depthState{ GetDepthState() };
    const Render::RasterizerState rasterizerState{ GetRasterizerState() };

    const Render::PipelineParams meshPipelineParams
    {
      .mProgram           { program },
      .mBlendState        { blendState },
      .mDepthState        { depthState },
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          {},
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "mesh-pso" },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL_RET( {}, Render::PipelineHandle meshPipeline{
      renderDevice->CreatePipeline( meshPipelineParams, errors ) }  );

    return meshPipeline;
  }

  // -----------------------------------------------------------------------------------------------

  static bool                sInitialized;
  static RenderMaterialCache sRenderMaterialCache;


  // -----------------------------------------------------------------------------------------------


  void            RenderMaterial::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DShader );
    renderDevice->DestroyPipeline( mMeshPipeline );
  }

  // -----------------------------------------------------------------------------------------------

  RenderMaterial* RenderMaterialApi::GetRenderMaterial( const Material* material,
                                                        Errors& errors )
  {
    if( RenderMaterial * renderMaterial{ sRenderMaterialCache.Find( material ) } )
      return renderMaterial;

    // TODO: async
    TAC_CALL_RET( {}, const ShaderGraph shaderGraph{
      ShaderGraph::FromPath( material->mShaderGraph, errors ) } );

    TAC_CALL_RET( {}, const Render::ProgramHandle program{
      Create3DShader( shaderGraph, errors ) } );

    TAC_CALL_RET( {}, const Render::PipelineHandle meshPipeline{
      CreatePipeline( program, errors ) } );

    const HashValue hashValue{ Hash( ( StringView )material->mShaderGraph ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    Render::IShaderVar* perFrame { renderDevice->GetShaderVariable( meshPipeline, "sPerObj" ) };
    Render::IShaderVar* perObj   { renderDevice->GetShaderVariable( meshPipeline, "sPerFrame" ) };


    const RenderMaterial renderMaterial
    {
      .mShaderGraphPath    { material->mShaderGraph },
      .mShaderGraph        { shaderGraph },
      .mMaterialShaderHash { hashValue },
      .m3DShader           { program },
      .mMeshPipeline       { meshPipeline },
      .mShaderVarPerFrame  { perFrame },
      .mShaderVarPerObject { perObj },
    };

    return &( sRenderMaterialCache.mRenderMaterials.emplace_back() = renderMaterial );
  }

  void            RenderMaterialApi::Init()
  {
    sInitialized = true;
  }

  void            RenderMaterialApi::Uninit()
  {
    sRenderMaterialCache.Clear();

    sInitialized = {};
  }

} // namespace Tac::Render
