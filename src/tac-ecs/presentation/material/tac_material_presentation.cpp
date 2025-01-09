#include "tac_material_presentation.h" // self-inc

#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/material/tac_render_material.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"

#if TAC_MATERIAL_PRESENTATION_ENABLED()

namespace Tac
{
  struct MaterialFlags
  {
    enum Flag
    {
      kDefault,
      kIsGLTF_PBR_MetallicRoughness,
      kIsGLTF_PBR_SpecularGlossiness,
      kCount,
    };

    static_assert( Flag::kCount < 32 );

    bool IsSet( Flag f ) const         { return mFlags & ( 1 << f ); }
    void Set( Flag f )                 { mFlags |= ( 1 << f ); }
    void Set( Flag f, bool b )         { if( b ) Set( f ); else Clear( f ); }
    void Clear()                       { mFlags = 0; }
    void Clear( Flag f )               { mFlags &= ~( 1 << f ); }

    u32 mFlags {};
  };

  // comment please
  //
  // uhh it looks like this is data that is sent to every shader irregardless of material or
  // shader graph. so its just always available data
  struct ConstBufData_PerFrame
  {
    m4 mWorldToClip    {};
  };

  struct ConstBufData_ShaderGraph
  {
    struct Params
    {
      const Entity*                 mEntity         {};
      const Model*                  mModel          {};
      const Mesh*                   mMesh           {};
      const SubMesh*                mSubMesh        {};
      const Material*               mMaterial       {};
      const Render::RenderMaterial* mRenderMaterial {};
    };
    static ConstBufData_ShaderGraph Get( const Params& );
    m4  mWorld              {};
    u32 mVertexBufferIndex  { ( u32 )-1 };
    u32 mInputLayoutIndex   { ( u32 )-1 };
  };

  struct ConstBufData_Material
  {
    v4            mColor              {};
    v4            mEmissive           {};
    MaterialFlags mFlags              {};
    u32           mDiffuseTextureIdx  {};
    u32           mSpecularTextureIdx {};
  };

  // -----------------------------------------------------------------------------------------------

  Render::BufferHandle MaterialPresentation::sConstBufHandle_PerFrame    {};
  Render::BufferHandle MaterialPresentation::sConstBufHandle_Material    {};
  Render::BufferHandle MaterialPresentation::sConstBufHandle_ShaderGraph {};
  static bool          sEnabled                                          { true };
  static bool          sInitialized                                      {};

  // -----------------------------------------------------------------------------------------------

  static m4                    GetProjMtx( const Camera* camera, const v2i viewSize )
  {
    const float aspectRatio{ ( float )viewSize.x / ( float )viewSize.y };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    const m4 proj{ m4::ProjPerspective( projParams ) };
    return proj;
  }

  static ConstBufData_PerFrame GetPerFrameParams( const Camera* camera, const v2i viewSize )
  {
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    const m4 worldToClip{ proj * view };
    return ConstBufData_PerFrame
    {
      .mWorldToClip    { worldToClip },
    };
  }

  static ConstBufData_Material GetMaterialParams( const Material* material, Errors& errors )
  {
    MaterialFlags flags;
    flags.Set( MaterialFlags::kIsGLTF_PBR_MetallicRoughness,
               material->mIsGlTF_PBR_MetallicRoughness );
    flags.Set( MaterialFlags::kIsGLTF_PBR_SpecularGlossiness,
               material->mIsGlTF_PBR_SpecularGlossiness );

    TAC_CALL_RET( Render::IBindlessArray::Binding diffuse{
      TextureAssetManager::GetBindlessIndex( material->mTextureDiffuse, errors ) } );

    TAC_CALL_RET( Render::IBindlessArray::Binding specular{
      TextureAssetManager::GetBindlessIndex( material->mTextureSpecular, errors ) } );

    /*
    bool            material->mIsGlTF_PBR_MetallicRoughness  {};
    bool            material->mIsGlTF_PBR_SpecularGlossiness {};
    float           material->mPBR_Factor_Metallic           {};
    float           material->mPBR_Factor_Roughness          {};
    v3              material->mPBR_Factor_Diffuse            {};
    v3              material->mPBR_Factor_Specular           {};
    float           material->mPBR_Factor_Glossiness         {};
    v4              material->mColor                         {};
    v3              material->mEmissive                      {};
    AssetPathString material->mTextureDiffuse                {};
    AssetPathString material->mTextureSpecular               {};
    AssetPathString material->mTextureGlossiness             {};
    AssetPathString material->mTextureMetallic               {};
    AssetPathString material->mTextureRoughness              {};
    */

    return ConstBufData_Material
    {
      .mColor              { material->mColor },
      .mEmissive           { material->mEmissive, 1 },
      .mFlags              { flags },
      .mDiffuseTextureIdx  { ( u32 )diffuse.GetIndex() },
      .mSpecularTextureIdx { ( u32 )specular.GetIndex() },
    };
  }

  static Render::BufferHandle  CreateDynCBuf( int byteCount, const char* name, Errors& errors )
  {
    const Render::CreateBufferParams params
    {
      .mByteCount     { byteCount },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { name },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateBuffer( params, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void             MaterialPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;

    Render::RenderMaterialApi::Init();

    TAC_CALL( sConstBufHandle_PerFrame = CreateDynCBuf( sizeof( ConstBufData_PerFrame ),
                                                        "material-presentation-per-frame",
                                                        errors ) );

    TAC_CALL( sConstBufHandle_Material = CreateDynCBuf( sizeof( ConstBufData_Material ),
                                                        "material-presentation-material",
                                                        errors ) );

    TAC_CALL( sConstBufHandle_ShaderGraph = CreateDynCBuf( sizeof( ConstBufData_ShaderGraph ),
                                                           "material-presentation-shader-graph",
                                                           errors ) );

    sInitialized = true;
  }

  void             MaterialPresentation::Uninit()
  {
    if( sInitialized )
    {
      Render::RenderMaterialApi::Uninit();
      sInitialized = false;
    }
  }

  void             MaterialPresentation::Render( Render::IContext* renderContext,
                                                 const World* world,
                                                 const Camera* camera,
                                                 const v2i viewSize,
                                                 const Render::TextureHandle dstColorTex,
                                                 const Render::TextureHandle dstDepthTex,
                                                 Errors& errors )
  {
    TAC_ASSERT( sInitialized );
    if( !sEnabled )
      return;

    const Render::Targets renderTargets
    {
      .mColors { dstColorTex },
      .mDepth  { dstDepthTex },
    };

    TAC_RENDER_GROUP_BLOCK( renderContext, "materialpresentation::render" );

    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetRenderTargets( renderTargets );

    const ConstBufData_PerFrame constBufData_PerFrame{
      GetPerFrameParams( camera, viewSize ) };
    TAC_CALL( renderContext->UpdateBufferSimple( sConstBufHandle_PerFrame,
                                                 constBufData_PerFrame,
                                                 errors ) );

    for( const Entity* entity : world->mEntities )
    {
      const Model* model{ Model::GetModel( entity ) };
      if( !model )
        continue;

      const Material* material{ Material::GetMaterial( entity ) };
      if( !material )
        continue;

      if( material->mShaderGraph.empty() )
        continue;

      if( !material->mRenderEnabled )
        continue;

      TAC_CALL( const Mesh * mesh{
        ModelAssetManager::GetMesh(
          ModelAssetManager::Params
          {
            .mPath        { model->mModelPath.c_str() },
            .mModelIndex  { model->mModelIndex },
          },
          errors ) } );
      if( !mesh )
        return;

      TAC_RENDER_GROUP_BLOCK( renderContext, model->mEntity->mName );

      TAC_CALL( Render::RenderMaterial* renderMaterial{
        Render::RenderMaterialApi::GetRenderMaterial( material, errors ) } );


      TAC_CALL( const ConstBufData_Material constBufData_Material{
        GetMaterialParams( material, errors ) } );
      TAC_CALL( renderContext->UpdateBufferSimple( sConstBufHandle_Material,
                                                   constBufData_Material,
                                                   errors ) );



      renderContext->SetPipeline( renderMaterial->mMeshPipeline );

      // [ ] Q: Do uhh, do all the submeshes share the same material?
      //        
      //        in unity3d, you can only use multiple materials if
      //        a meshrenderer has multiple submeshes

      const int nSubMeshes{ mesh->mSubMeshes.size() };
      for( int iSubMesh{}; iSubMesh < nSubMeshes; ++iSubMesh )
      {
        const ShortFixedString subMeshEventName{ [&](){
          if( !kIsDebugMode || nSubMeshes == 1 )
            return ShortFixedString{};
          return ShortFixedString::Concat( model->mEntity->mName,
                                           "(submesh ",
                                           ToString( iSubMesh ),
                                           " of ",
                                           ToString( nSubMeshes ),
                                           ")" );
          }() };

        if( !subMeshEventName.empty() )
          renderContext->DebugEventBegin( subMeshEventName );

        const SubMesh& subMesh { mesh->mSubMeshes[ iSubMesh ] };
        const Render::DrawArgs drawArgs
        {
          .mVertexCount { subMesh.mVertexCount },
          .mIndexCount  { subMesh.mIndexCount },
        };

        const ConstBufData_ShaderGraph constBufData_ShaderGraph{
          ConstBufData_ShaderGraph::Get(
            ConstBufData_ShaderGraph::Params
            {
              .mEntity         { entity },
              .mModel          { model },
              .mMesh           { mesh },
              .mSubMesh        { &subMesh },
              .mMaterial       { material },
              .mRenderMaterial { renderMaterial },
            } ) };
        TAC_CALL( renderContext->UpdateBufferSimple( sConstBufHandle_ShaderGraph,
                                                     constBufData_ShaderGraph,
                                                     errors ) );

        renderContext->CommitShaderVariables();

        renderContext->DebugMarker( subMesh.mName );
        // | we bindless now
        // v
        //renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
        renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
        renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
        renderContext->Draw( drawArgs );

        if( !subMeshEventName.empty() )
          renderContext->DebugEventEnd();
      }
    }

  }

  void             MaterialPresentation::DebugImGui()
  {
    ImGuiCheckbox( "Material Presentation", &sEnabled );
  }

  // -----------------------------------------------------------------------------------------------

  ConstBufData_ShaderGraph ConstBufData_ShaderGraph::Get( const Params& params )
  {
    struct Handle
    {
      //static void Noop( const Params& , dynmc ConstBufData_ShaderGraph* ) {  }// Do nothing

      static void WorldMatrix( const Params& params, dynmc ConstBufData_ShaderGraph* shaderGraph )
      {
        shaderGraph->mWorld = params.mEntity->mWorldTransform;
      }

      static void VertexBuffer( const Params& params, dynmc ConstBufData_ShaderGraph* shaderGraph )
      {
        TAC_ASSERT( params.mSubMesh->mVertexBufferBinding.IsValid() );
        shaderGraph->mVertexBufferIndex = ( u32 )params.mSubMesh->mVertexBufferBinding.GetIndex();
      }

      static void InputLayout( const Params& params, dynmc ConstBufData_ShaderGraph* shaderGraph )
      {
        TAC_ASSERT( params.mMesh->mGPUInputLayoutBinding.IsValid() );
        shaderGraph->mInputLayoutIndex = ( u32 )params.mMesh->mGPUInputLayoutBinding.GetIndex();
      }
    };

    using HandleFn = void ( * )( const Params&, dynmc ConstBufData_ShaderGraph* );

    HandleFn mHandlers[ sizeof( MaterialInput::UnderlyingType ) * 8 ]{};
    //mHandlers[ ( int )MaterialInput::Type::kUnknown ] = Handle::Noop;
    mHandlers[ ( int )MaterialInput::Type::kVertexBuffer ] = Handle::VertexBuffer;
    mHandlers[ ( int )MaterialInput::Type::kWorldMatrix ] = Handle::WorldMatrix;
    mHandlers[ ( int )MaterialInput::Type::kInputLayout ] = Handle::InputLayout;

    ConstBufData_ShaderGraph shaderGraph{};

    const MaterialInput& materialInput{ params.mRenderMaterial->mShaderGraph.mMaterialInputs };
    for( const MaterialInput::Type type : MaterialInput::TypeIterator() )
    {
      if( materialInput.IsSet( type ) )
      {
        const HandleFn handler{ mHandlers[ ( int )type ] };
        TAC_ASSERT( handler );
        handler( params, &shaderGraph );
      }
    }

    return shaderGraph;

  }

} // namespace Tac

#endif // TAC_MATERIAL_PRESENTATION_ENABLED
