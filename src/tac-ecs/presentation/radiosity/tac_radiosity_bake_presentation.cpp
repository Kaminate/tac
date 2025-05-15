#include "tac_radiosity_bake_presentation.h" // self-inc
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/material/tac_material.h"

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <limits>
#endif

namespace Tac
{
  static bool sInitialized;

  // -----------------------------------------------------------------------------------------------

  struct PreBakeScene
  {
    struct PatchPower
    {
      using Vtxs = Array< v3, 3 >;

      Vtxs mTriVerts             {}; // worldspace
      v3   mTotalPower           {};
      v3   mCurrentReceivedPower {};
      v3   mCurrentUnshotPower   {};
    };

    struct Instance
    {
      using PatchPowers = Vector< PatchPower >;

      const Entity*   mEntity      {};
      const Model*    mModel       {};
      const Mesh*     mMesh        {};
      const Material* mMaterial    {};
      PatchPowers     mPatchPowers {};
    };

    struct RaycastResult
    {
      PatchPower*     mHitPatch         {};
      const Material* mHitPatchMaterial {};
    };

    RaycastResult Raycast(PatchPower* fromPatch, RayTriangle::Ray ray ) // worldspace
    {
      RaycastResult result;
      result.mHitPatch;
      result.mHitPatchMaterial;
      dynmc float minT { std::numeric_limits<float>::max()};

      for(Instance& instance : mInstances)
      {
        for( PatchPower& patchPower : instance.mPatchPowers )
        {
          if( &patchPower == fromPatch )
            continue;

          const RayTriangle::Triangle triangle
          {
            .mP0 { patchPower.mTriVerts[0] },
            .mP1 { patchPower.mTriVerts[1] },
            .mP2 { patchPower.mTriVerts[2] },
          };
          const RayTriangle::Output raycastResult { RayTriangle::Solve( ray, triangle )};
          if( !raycastResult.mValid || raycastResult.mT >= minT )
            continue;

          result.mHitPatch = &patchPower;
          result.mHitPatchMaterial = instance.mMaterial;
          minT = raycastResult.mT;
        }
      }
    }

    const Mesh* GetMesh(const Model* model)
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
        const Model * model { Model::GetModel( entity ) };
        if(!model)
          continue;

        const Mesh* mesh { GetMesh(model) };
        const Material* material { Material::GetMaterial(entity)};
        const JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };

        //dynmc bool worldTransformInvExists{};
        const m4 worldTransform{ model->mEntity->mWorldTransform };
        //const m4 worldTransformInv{ m4::Inverse( worldTransform, &worldTransformInvExists ) };
        //TAC_ASSERT( worldTransformInvExists );

        const Instance::PatchPowers patchPowers{ [ & ]()
        {
          dynmc Instance::PatchPowers patchPowers;
          dynmc int iTriVert{};
          dynmc PatchPower::Vtxs triVerts{};
          for( const v3& vert : jpptCPUMeshData.mPositions )
          {
            triVerts[ iTriVert ] = (worldTransform * v4(vert, 1)).xyz();
            if( iTriVert == 2 )
            {
              const PatchPower patchPower
              {
                .mTriVerts           { triVerts },
                .mTotalPower         { material->mEmissive },
                .mCurrentUnshotPower { material->mEmissive },
              };
              patchPowers.push_back( patchPower );
            }
            ( ++iTriVert ) %= 3;
          }
          return patchPowers;
          }( ) };

        Instance instance
        {
          .mEntity      { entity },
          .mModel       { model },
          .mMesh        { mesh },
          .mMaterial    { material },
          .mPatchPowers { patchPowers },
        };
        mInstances.push_back(instance);
      }
    }

    v3 ComputeTotalUnshotPower()
    {
      v3 totalUnshotPower;
      for( Instance& instance : mInstances )
        for( PatchPower& patchPower : instance.mPatchPowers )
          totalUnshotPower += patchPower.mCurrentUnshotPower;
      return totalUnshotPower;
    }

    void Bake()
    {
      float minPowerLimit{ 0.01f };
      int maxIterations{ 100 };

      for( int iter{}; iter < maxIterations; ++iter )
      {
        const v3 totalUnshotPower{ ComputeTotalUnshotPower() };

        if( totalUnshotPower.Length() < minPowerLimit )
          break;

        TAC_NO_OP;

        for( Instance& instance : mInstances)
        {
          for( PatchPower& patchPower : instance.mPatchPowers)
          {
            //float q_i = patchPower.mCurrentUnshotPower / totalUnshotPower;
          }
        }
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
      sRequestBake = false;
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
