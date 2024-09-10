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
  struct MaterialPerFrameBuf
  {
    m4 mView    {};
    m4 mProj    {};
  };

  struct MaterialPerObjBuf
  {
    m4  mWorld    {};
    u32 mIVtxBuf  { ( u32 )-1 };
    u32 mIMtlBuf  { ( u32 )-1 };
  };

  struct MaterialBufferData
  {
    struct Flags
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
    
    v4    mColor    {};
    v4    mEmissive {};
    Flags mFlags    {};
  };

  static Render::BufferHandle          mMaterialPerFrameBuf {};
  static Render::BufferHandle          mMaterialPerObjBuf   {};
  static bool                          sEnabled             { false };
  static bool                          sInitialized         {};

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

  static MaterialPerFrameBuf GetPerFrameBuf( const Camera* camera,
                                         const v2i viewSize )
  {
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    return MaterialPerFrameBuf
    {
      .mView    { view },
      .mProj    { proj },
    };
  }

  static MaterialPerObjBuf GetPerObjBuf( const Model* model,
                                         const Material* material )// <-- todo: use 
  {
    const m4 world { model->mEntity->mWorldTransform };

    TAC_ASSERT_UNIMPLEMENTED; // .mIVtxBuf , .mIMtlBuf ,
    return MaterialPerObjBuf
    {
      .mWorld { world },
      .mIVtxBuf {},
      .mIMtlBuf {},
    };
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

    TAC_CALL( mMaterialPerFrameBuf = CreateDynCBuf( sizeof( MaterialPerFrameBuf ),
                                                    "material-per-frame-cbuf",
                                                    errors ) );

    TAC_CALL( mMaterialPerObjBuf = CreateDynCBuf( sizeof( MaterialPerObjBuf ),
                                                 "material-per-obj-cbuf",
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

    const MaterialPerFrameBuf perFrameData{ GetPerFrameBuf( camera, viewSize ) };
    TAC_CALL( renderContext->UpdateBufferSimple( mMaterialPerFrameBuf, perFrameData, errors ) );

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

      const MaterialPerObjBuf perObj{ GetPerObjBuf( model ,material ) };
      TAC_CALL( renderContext->UpdateBufferSimple( mMaterialPerObjBuf, perObj, errors ) );


      // $$$ gross
      if( !renderMaterial->mAreShaderVarsSet )
      {
        renderMaterial->mShaderVarPerFrame->SetBuffer( mMaterialPerFrameBuf );
        renderMaterial->mShaderVarPerObject->SetBuffer( mMaterialPerObjBuf );
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
