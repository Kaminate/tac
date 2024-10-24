#include "tac_material_presentation.h" // self-inc

#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/material/tac_render_material.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"

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

    bool IsSet( Flag f ) const { return mFlags & ( 1 << f ); }
    void Set( Flag f )         { mFlags |= ( 1 << f ); }
    void Set( Flag f, bool b ) { if( b ) Set( f ); else Clear( f ); }
    void Clear()               { mFlags = 0; }
    void Clear( Flag f )       { mFlags &= ~( 1 << f ); }

    u32 mFlags {};
  };

  // comment please
  struct ConstBufData_PerFrame
  {
    m4 mWorldToClip    {};
  };

  struct ConstBufData_ShaderGraph
  {
    static ConstBufData_ShaderGraph Get( const Entity*,
                                         const Model*,
                                         const Material*,
                                         const Render::RenderMaterial* );
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

  static Render::BufferHandle          sConstBufHandle_PerFrame    {};
  static Render::BufferHandle          sConstBufHandle_Material    {};
  static Render::BufferHandle          sConstBufHandle_ShaderGraph {};
  static bool                          sEnabled                    { true };
  static bool                          sInitialized                {};

  static m4 GetProjMtx( const Camera* camera, const v2i viewSize )
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

  static ConstBufData_PerFrame GetPerFrameParams( const Camera* camera,
                                         const v2i viewSize )
  {
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    const m4 worldToClip{ proj * view };
    return ConstBufData_PerFrame
    {
      .mWorldToClip    { worldToClip },
    };
  }

  static ConstBufData_Material GetMaterialParams( const Model* model,
                                         const Material* material )// <-- todo: use 
  {
    return ConstBufData_Material
    {
      .mColor              {},
      .mEmissive           {},
      .mFlags              {},
      .mDiffuseTextureIdx  {},
      .mSpecularTextureIdx {},
    };
  }

  ConstBufData_ShaderGraph
    ConstBufData_ShaderGraph::Get( const Entity* entity,
                                   const Model* model,
                                   const Material* material,
                                   const Render::RenderMaterial* renderMaterial )
  {

    struct Getter
    {
      ConstBufData_ShaderGraph Get( const Entity* entity,
                                    const Model* model,
                                    const Material* material,
                                    const Render::RenderMaterial* renderMaterial )
      {
        ConstBufData_ShaderGraph result{};

        const Params params
        {
          .mEntity         { entity },
          .mModel          { model },
          .mMaterial       { material },
          .mRenderMaterial { renderMaterial },
          .mShaderGraph    { &result },
        };

        const MaterialInput& materialInput{ renderMaterial->mShaderGraph.mMaterialInputs };

        const int n{ ( int )MaterialInput::Type::kCount };
        for( int i{}; i < n; ++i )
        {
          const MaterialInput::Type type{ ( MaterialInput::Type )i };
          if( materialInput.IsSet( type ) )
          {
            const Handler handler{ mHandlers[ i ] };
            handler( params );
          }
        }

        return result;
      }

      Getter()
      {
        mHandlers[ ( int )MaterialInput::Type::kVertexBuffer ] = Handle_VertexBuffer;
        mHandlers[ ( int )MaterialInput::Type::kWorldMatrix ] = Handle_WorldMatrix;
        mHandlers[ ( int )MaterialInput::Type::kInputLayout ] = Handle_InputLayout;

        for( Handler handler : mHandlers )
        {
          TAC_ASSERT( handler );
        }
      }

    private:

      struct Params
      {
        // input
        const Entity* mEntity;
        const Model* mModel;
        const Material* mMaterial;
        const Render::RenderMaterial* mRenderMaterial;

        // output
        dynmc ConstBufData_ShaderGraph* mShaderGraph;
      };

      using Handler = void ( * )( const Params& );

      static void Handle_WorldMatrix( const Params& params )
      {
        params.mShaderGraph->mWorld = params.mEntity->mWorldTransform;
      }

      static void Handle_VertexBuffer( const Params& params )
      {
        params.mShaderGraph->mVertexBufferIndex = ( u32 )-1;
      }

      static void Handle_InputLayout( const Params& params )
      {
        params.mShaderGraph->mInputLayoutIndex = ( u32 )-1;
      }


      Handler mHandlers[ ( int )MaterialInput::Type::kCount ]{};
    };

    static Getter getter;

    return getter.Get( entity, model, material, renderMaterial );
  }


  static Render::BufferHandle CreateDynCBuf( int byteCount, const char* name, Errors& errors )
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

    TAC_RENDER_GROUP_BLOCK( renderContext, __func__ );
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

      const Render::VertexDeclarations vtxDecls{};

      const ModelAssetManager::Params meshParams
      {
        .mPath        { model->mModelPath.c_str() },
        .mModelIndex  { model->mModelIndex },
        .mOptVtxDecls { vtxDecls },
      };

      TAC_CALL( const Mesh * mesh{ ModelAssetManager::GetMesh( meshParams, errors ) } );
      if( !mesh )
        return;

      TAC_RENDER_GROUP_BLOCK( renderContext, model->mEntity->mName );

      TAC_CALL( Render::RenderMaterial* renderMaterial{
        Render::RenderMaterialApi::GetRenderMaterial( material, errors ) } );

      const ConstBufData_ShaderGraph constBufData_ShaderGraph{
        ConstBufData_ShaderGraph::Get( entity, model ,material, renderMaterial ) };
      TAC_CALL( renderContext->UpdateBufferSimple( sConstBufHandle_ShaderGraph,
                                                   constBufData_ShaderGraph,
                                                   errors ) );

      const ConstBufData_Material constBufData_Material{
        GetMaterialParams( model ,material ) };
      TAC_CALL( renderContext->UpdateBufferSimple( sConstBufHandle_Material,
                                                   constBufData_Material,
                                                   errors ) );


      // $$$ gross
      if( !renderMaterial->mAreShaderVarsSet )
      {
        renderMaterial->mShaderVar_PerFrame->SetResource( sConstBufHandle_PerFrame );
        renderMaterial->mShaderVar_Material->SetResource( sConstBufHandle_Material );
        renderMaterial->mShaderVar_ShaderGraph->SetResource( sConstBufHandle_ShaderGraph );

        mesh->mGPUInputLayoutBuffer;

        TAC_ASSERT_UNIMPLEMENTED;

        //renderMaterial->mShaderVar_Buffers->SetBufferAtIndex( ... );
        renderMaterial->mAreShaderVarsSet = true;
      }

      renderContext->SetPipeline( renderMaterial->mMeshPipeline );
      renderContext->CommitShaderVariables();

      const int nSubMeshes{ mesh->mSubMeshes.size() };
      for( const SubMesh& subMesh : mesh->mSubMeshes )
      {
        const Render::DrawArgs drawArgs
        {
          .mVertexCount { subMesh.mVertexCount },
          .mIndexCount  { subMesh.mIndexCount },
        };

        renderContext->DebugMarker( subMesh.mName );
        renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
        renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
        renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
        renderContext->Draw( drawArgs );
      }
    }

  }

  void             MaterialPresentation::DebugImGui()
  {
    ImGuiCheckbox( "Material Presentation", &sEnabled );
  }

  // -----------------------------------------------------------------------------------------------

}

#endif
