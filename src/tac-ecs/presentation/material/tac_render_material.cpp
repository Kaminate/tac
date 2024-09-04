#include "tac_render_material.h" // self-inc

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  static Render::DepthState GetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static Render::BlendState GetBlendState()
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

  static Render::ProgramHandle Create3DShader( const Material* material, Errors& errors )
  {
    const StringView materialShader{ material->mMaterialShader };

    TAC_UNUSED_PARAMETER( errors );

    const Vector< String > inputs
    {
      material->mMaterialShader,
      "Material.hlsl",
    };

      /*

      +----------+
      | material | shader graph
      +----------+
        |
        +-- Define required vtx inputs (pos3, nor3, uv2, etc)
        |
        +-- this is fed into shader creation, which defines VS_OUT attributes


        Goals for the material system:

          - Be able to have shadertoy pixel shaders

          - Support multiple input layouts. Although we need to change PSOs, this is performant
            because thats what PSOs were made for
      */

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs     { inputs },
      .mStackFrame { TAC_STACK_FRAME },
    };
    return renderDevice->CreateProgram( programParams, errors );
  }

  static Render::PipelineHandle CreatePipeline( Render::ProgramHandle program,
                                                Errors& errors )
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

  static Vector< RenderMaterial >         mRenderMaterials;
  static bool                             sInitialized;

  static RenderMaterial* FindRenderMaterial( const Material* material )
  {
    const StringView materialShader{ material->mMaterialShader };
    const HashValue hashValue{ Hash( materialShader ) };
    for ( RenderMaterial& renderMaterial : mRenderMaterials )
    {
      if ( renderMaterial.mMaterialShaderHash == hashValue &&
        ( StringView )renderMaterial.mMaterialShader == materialShader )
      {
        return &renderMaterial;
      }
    }

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------


  void            RenderMaterial::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DShader );
    renderDevice->DestroyPipeline( mMeshPipeline );
  }

  // -----------------------------------------------------------------------------------------------

  RenderMaterial*                   RenderMaterialApi::GetRenderMaterial( const Material* material,
                                                                          Errors& errors )
  {
    if( RenderMaterial * renderMaterial{ FindRenderMaterial( material ) } )
      return renderMaterial;

    TAC_CALL_RET( {}, Render::ProgramHandle program{ Create3DShader( material, errors ) } );
    TAC_CALL_RET( {}, Render::PipelineHandle meshPipeline{ CreatePipeline( program, errors ) } );

    const HashValue hashValue{ Hash( material->mMaterialShader ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    Render::IShaderVar* perFrame { renderDevice->GetShaderVariable( meshPipeline, "sPerObj" ) };
    Render::IShaderVar* perObj   { renderDevice->GetShaderVariable( meshPipeline, "sPerFrame" ) };

    return &( mRenderMaterials.emplace_back() = RenderMaterial
              {
                .mMaterialShader     { material->mMaterialShader },
                .mMaterialShaderHash { hashValue },
                .m3DShader           { program },
                .mMeshPipeline       { meshPipeline },
                .mShaderVarPerFrame  { perFrame },
                .mShaderVarPerObject { perObj },
              } );
  }

  void                              RenderMaterialApi::Init()
  {
    sInitialized = true;
  }

  void                              RenderMaterialApi::Uninit()
  {
    for( RenderMaterial& renderMaterial : mRenderMaterials )
      renderMaterial.Uninit();

    mRenderMaterials = {};
    sInitialized = {};
  }

} // namespace Tac::Render
