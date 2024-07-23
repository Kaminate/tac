#include "tac_render_material.h" // self-inc

namespace Tac::Render
{
  struct MeshModelVtx
  {
    v3 mPos;
    v3 mNor;
  };

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

  static Render::VertexDeclarations CreateVertexDeclarations()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mNor ) },
    };

    Render::VertexDeclarations decls;
    decls.push_back( posDecl );
    decls.push_back( norDecl );
    return decls;
  }

  static Render::ProgramHandle Create3DShader( StringView fileStem, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::ProgramParams programParams
    {
      .mFileStem   { fileStem },
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
    const Render::VertexDeclarations vtxDecls{ CreateVertexDeclarations() };

    const Render::PipelineParams meshPipelineParams
    {
      .mProgram           { program },
      .mBlendState        { blendState },
      .mDepthState        { depthState },
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { vtxDecls },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "mesh-pso" },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL_RET( {}, Render::PipelineHandle meshPipeline{
      renderDevice->CreatePipeline( meshPipelineParams, errors ) }  );

    return meshPipeline;
  }

  // -----------------------------------------------------------------------------------------------

  static Render::VertexDeclarations       sVtxDecls;
  static Vector< RenderMaterial >         mRenderMaterials;
  static bool                             sInitialized;

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
    const StringView materialShader{ material->mMaterialShader };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const HashValue hashValue{ Hash( materialShader ) };
    for ( RenderMaterial& renderMaterial : mRenderMaterials )
    {
      if ( renderMaterial.mMaterialShaderHash == hashValue &&
        ( StringView )renderMaterial.mMaterialShader == materialShader )
      {
        return &renderMaterial;
      }
    }

    TAC_CALL_RET( {}, Render::ProgramHandle program{ Create3DShader( materialShader, errors ) } );
    TAC_CALL_RET( {}, Render::PipelineHandle meshPipeline{ CreatePipeline( program, errors ) } );

    Render::IShaderVar* shaderVarPerFrame  {
      renderDevice->GetShaderVariable( meshPipeline, "sPerObj" ) };
    Render::IShaderVar* shaderVarPerObject {
      renderDevice->GetShaderVariable( meshPipeline, "sPerFrame" ) };

    mRenderMaterials.resize( mRenderMaterials.size() + 1 );
    RenderMaterial& renderMaterial{ mRenderMaterials.back() };
    renderMaterial = RenderMaterial
    {
      .mMaterialShader     { materialShader },
      .mMaterialShaderHash { hashValue },
      .m3DShader           { program },
      .mMeshPipeline       { meshPipeline },
      .mShaderVarPerFrame  { shaderVarPerFrame },
      .mShaderVarPerObject { shaderVarPerObject },
    };

    return &renderMaterial;
  }

  const Render::VertexDeclarations& RenderMaterialApi::GetVertexDeclarations()
  {
    return sVtxDecls;
  }

  void                              RenderMaterialApi::Init()
  {
    sVtxDecls = CreateVertexDeclarations();
    sInitialized = true;
  }

  void                              RenderMaterialApi::Uninit()
  {
    for( RenderMaterial& renderMaterial : mRenderMaterials )
      renderMaterial.Uninit();

    mRenderMaterials = {};
  }

} // namespace Tac::Render
