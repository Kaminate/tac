#include "tac_radiosity_bake_presentation.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/material/tac_material.h"

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

namespace Tac
{
  static bool sInitialized;

  // -----------------------------------------------------------------------------------------------

  struct PreBakeScene
  {
    struct Instance
    {
      const Entity*   mEntity {};
      const Mesh*     mMesh   {};
      const Model*    mModel  {};
      const Material* mMaterial {};
      const Light*    mLight  {};
    };

    Mesh* GetMesh(const Model* model)
    {
      Mesh* mesh{};
      while( !mesh )
      {
        Errors errors;
        const ModelAssetManager::Params getMeshParams
        {
          .mPath       { model->mModelPath },
          .mModelIndex { model->mModelIndex },
        };
        mesh = ModelAssetManager::GetMesh( getMeshParams, errors );
        TAC_ASSERT( !errors );
        OS::OSThreadSleepMsec( 1 ); // wait for it to load
      }
      return mesh;
    }

    void Init(const World* world)
    {
      Vector< const Model* > models;
      for( const Entity* entity : world->mEntities )
      {
        const Model * model = Model::GetModel( entity );
        if(!model)
          continue;

        Mesh* mesh = GetMesh(model);

        JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };

        dynmc bool worldTransformInvExists{};
        const m4 worldTransform{ model->mEntity->mWorldTransform };
        const m4 worldTransformInv{ m4::Inverse( worldTransform, &worldTransformInvExists ) };
        TAC_ASSERT( worldTransformInvExists );

        Instance instance;
        instance.mEntity = entity;
        instance.mModel = model;
        instance.mMesh = mesh;
        instance.mLight = nullptr;
        instance.mMaterial = Material::GetMaterial(entity);
        mInstances.push_back(instance);
      }
    }

    Vector< Instance > mInstances;
  };

  static PreBakeScene sPreBakeScene;
  static bool         sRequestBake;

  // -----------------------------------------------------------------------------------------------

  void RadiosityBakePresentation::Init( Errors& )
  {
    if( sInitialized )
      return;

    sInitialized = true;

#if 0
    if( sPipeline.IsValid() )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs{ "jppt" },
    };

    TAC_CALL( sProgram = renderDevice->CreateProgram( programParams, errors ) );
    const Render::PipelineParams pipelineParams
    {
      .mProgram{ sProgram },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    Render::IShaderVar* outputTexture {
      renderDevice->GetShaderVariable( sPipeline, "sOutputTexture" ) };
    outputTexture->SetResource( sTexture );
#endif
  }

  void RadiosityBakePresentation::Render( Render::IContext* renderContext,
                                         const World* world,
                                         const Camera* camera,
                                         Errors& errors )
  {
    if(sRequestBake)
    {
      sPreBakeScene = {};
      sPreBakeScene.Init(world);
    }
  }

  void RadiosityBakePresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void RadiosityBakePresentation::DebugImGui()
  {
#if 0
    if( !ImGuiCollapsingHeader( "Radiosity Bake" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
#endif

    sRequestBake |= ImGuiButton( "Bake Radiosity" );
  }
}

#endif
