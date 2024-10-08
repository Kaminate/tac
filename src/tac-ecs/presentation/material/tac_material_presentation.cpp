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
    m4 mWorld {};
    v4 mColor {};
  };

  static Render::BufferHandle          mMaterialPerFrameBuf;
  static Render::BufferHandle          mMaterialPerObjBuf;
  static bool                          sEnabled{ true };
  static bool                          sInitialized;

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

  static void UpdatePerFrameCBuf( const Camera* camera,
                                  const v2i viewSize,
                                  Render::IContext* renderContext,
                                  Errors& errors )
  {
    const MaterialPerFrameBuf perFrameData{ GetPerFrameBuf( camera, viewSize ) };

    const Render::UpdateBufferParams updateBufferParams
    {
      .mSrcBytes     { &perFrameData },
      .mSrcByteCount { sizeof( MaterialPerFrameBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mMaterialPerFrameBuf, updateBufferParams, errors ) );
  }

  static void UpdatePerObjectCBuf( const Model* model,
                                   const Material* material,
                                   Render::IContext* renderContext,
                                   Errors& errors )
  {
    v4 color{ 1, 1, 1, 1 };

    const Render::DefaultCBufferPerObject perObjectData
    {
      .World { model->mEntity->mWorldTransform },
      .Color { Render::PremultipliedAlpha::From_sRGB_linearAlpha( color ) },
    };

    const Render::UpdateBufferParams updatePerObject
    {
      .mSrcBytes     { &perObjectData },
      .mSrcByteCount { sizeof( MaterialPerObjBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mMaterialPerObjBuf, updatePerObject, errors ) );
  }


  static void CreatePerMaterial( Errors& errors )
  {
    const Render::CreateBufferParams materialPerFrameParams
    {
      .mByteCount     { sizeof( MaterialPerFrameBuf ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "material-per-frame-cbuf" },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mMaterialPerFrameBuf = renderDevice->CreateBuffer( materialPerFrameParams, errors ) );
  }

  static void CreatePerObj( Errors& errors )
  {
    const Render::CreateBufferParams materialPerObjParams
    {
      .mByteCount     { sizeof( MaterialPerObjBuf ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "material-per-obj-cbuf" },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mMaterialPerObjBuf = renderDevice->CreateBuffer( materialPerObjParams, errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  void             MaterialPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;

    Render::RenderMaterialApi::Init();

    TAC_CALL( CreatePerMaterial( errors ) );
    TAC_CALL( CreatePerObj( errors ) );

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

    TAC_CALL( UpdatePerFrameCBuf( camera, viewSize, renderContext, errors ) );

    for( const Entity* entity : world->mEntities )
    {
      const Model* model{ Model::GetModel( entity ) };
      if( !model )
        continue;

      const Material* material{ Material::GetMaterial( entity ) };
      if( !material )
        continue;

      if( material->mMaterialShader.empty() )
        continue;

      if( !material->mRenderEnabled )
        continue;

      const Render::VertexDeclarations& vtxDecls{
        Render::RenderMaterialApi::GetVertexDeclarations() };

      TAC_CALL( const Mesh* mesh{
        ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                model->mModelIndex,
                                                vtxDecls,
                                                errors ) } );
      if( !mesh )
        return;

      TAC_RENDER_GROUP_BLOCK( renderContext, model->mEntity->mName );

      TAC_CALL( UpdatePerObjectCBuf( model, material, renderContext, errors ) );
      TAC_CALL( Render::RenderMaterial* renderMaterial{
        Render::RenderMaterialApi::GetRenderMaterial( material, errors ) } );

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
