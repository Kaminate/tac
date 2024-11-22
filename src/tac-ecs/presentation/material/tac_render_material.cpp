#include "tac_render_material.h" // self-inc

#include "tac_render_material_cache.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-ecs/presentation/material/tac_material_presentation.h"

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
    // subpath within Tac::Render::ProgramAttribs::mDir (no ext)
    const Vector< String > inputs
    {
      "material/preMaterialShader",
      shaderGraph.mMaterialShader,
      "material/postMaterialShader",
    };

    const String name{ [ & ]() { // remove folder from path
        const int i { shaderGraph.mMaterialShader.find_last_of( "/\\" ) };
        return i == String::npos
          ? shaderGraph.mMaterialShader
          : shaderGraph.mMaterialShader.substr( i + 1 );
      }( ) };


    const Render::ProgramParams programParams
    {
      .mName       { name },
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
    TAC_CALL_RET( Render::PipelineHandle meshPipeline{
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
    TAC_CALL_RET( const ShaderGraph shaderGraph{
      ShaderGraph::FileLoad( material->mShaderGraph, errors ) } );

    TAC_CALL_RET( const Render::ProgramHandle program{
      Create3DShader( shaderGraph, errors ) } );

    TAC_CALL_RET( const Render::PipelineHandle meshPipeline{
      CreatePipeline( program, errors ) } );

    const HashValue hashValue{ Hash( ( StringView )material->mShaderGraph ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    Render::IShaderVar* svPerFrame {
      renderDevice->GetShaderVariable( meshPipeline, "sPerFrameParams" ) };

    Render::IShaderVar* svMaterial{
      renderDevice->GetShaderVariable( meshPipeline, "sMaterialParams" ) };

    Render::IShaderVar* svShaderGraph{
      renderDevice->GetShaderVariable( meshPipeline, "sShaderGraphParams" ) };

    Render::IShaderVar* svBuffers{
      renderDevice->GetShaderVariable( meshPipeline, "sBuffers" ) };


    TAC_ASSERT(MaterialPresentation::sConstBufHandle_PerFrame.IsValid());
    svPerFrame->SetResource( MaterialPresentation::sConstBufHandle_PerFrame );

    TAC_ASSERT(MaterialPresentation::sConstBufHandle_Material.IsValid());
    svMaterial->SetResource( MaterialPresentation::sConstBufHandle_Material );

    TAC_ASSERT(MaterialPresentation::sConstBufHandle_ShaderGraph.IsValid());
    svShaderGraph->SetResource( MaterialPresentation::sConstBufHandle_ShaderGraph );

    Render::IBindlessArray* bindlessArray{ ModelAssetManager::GetBindlessArray() };
    TAC_ASSERT( bindlessArray );
    svBuffers->SetBindlessArray( bindlessArray );

    const RenderMaterial renderMaterial
    {
      .mShaderGraphPath            { material->mShaderGraph },
      .mShaderGraph                { shaderGraph },
      .mMaterialShaderHash         { hashValue },
      .m3DShader                   { program },
      .mMeshPipeline               { meshPipeline },
      .mShaderVar_cbuf_PerFrame    { svPerFrame },
      .mShaderVar_cbuf_Material    { svMaterial },
      .mShaderVar_cbuf_ShaderGraph { svShaderGraph },
      .mShaderVar_Buffers          { svBuffers },
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
