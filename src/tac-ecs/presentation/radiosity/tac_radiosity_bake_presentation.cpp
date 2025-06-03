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

  // Okay bro, heres the deal.
  //
  // I feel like using proper radiometric/photometric values suddenly makes things way complicated.
  // Like, for example, what radiance value should a light source emit?
  // In https://www.youtube.com/watch?v=B0sM7ZU0Nwo mirrors edge talk, they use a sun value
  // of 100000 lux. But then they have problems getting the correct white value.
  // Then theres exposure, tone mapping, etc.
  //
  // Instead, I think we should just go for it with SDR before even thinking about attempting HDR,
  // and use a light value of (1, 1, 1) in magical lighting units instead of some sort of physicaly
  // based W/(m^2sr) spectral radiance scRGB
  //
  // see also
  // https://seenaburns.com/dynamic-range/
  // http://www.gdcvault.com/play/1012351/Uncharted-2-HDR
  // http://filmicgames.com/archives/75
  // https://www.youtube.com/watch?v=B0sM7ZU0Nwo
  // https://bartwronski.com/2016/09/01/dynamic-range-and-evs/
  // https://placeholderart.wordpress.com/2014/11/21/implementing-a-physically-based-camera-manual-exposure/
  //  ^ this guy works at vicarious visions
  // https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/

  // -----------------------------------------------------------------------------------------------

  struct PreBakeScene
  {
    struct PatchPower
    {
      using Vtxs = Array< v3, 3 >;

      Vtxs  mTriVerts             {}; // worldspace
      v3    mTotalPower           {};
      v3    mCurrentReceivedPower {};
      v3    mCurrentUnshotPower   {};
      v3    mUnitNormal           {};
      float mArea                 {};
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
      for( const Entity* entity : world->mEntities )
      {
        const Model* model{ Model::GetModel( entity ) };
        if( !model )
          continue;

        const Mesh* mesh{ GetMesh( model ) };
        const Material* material{ Material::GetMaterial( entity ) };
        const JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };
        const m4 worldTransform{ model->mEntity->mWorldTransform };

        const Instance::PatchPowers patchPowers{ [ & ]()
        {
          dynmc Instance::PatchPowers patchPowers;
          dynmc int iTriVert{};
          dynmc PatchPower::Vtxs triVerts{};
          for( const v3& vert : jpptCPUMeshData.mPositions )
          {
            triVerts[ iTriVert ] = ( worldTransform * v4( vert, 1 ) ).xyz();
            if( iTriVert == 2 )
            {
              const v3 e1{ triVerts[ 1 ] - triVerts[ 0 ] };
              const v3 e2{ triVerts[ 2 ] - triVerts[ 0 ] };
              const v3 normal = Cross( e1, e2 );
              const float normalLen = normal.Length();
              const float area{ normalLen / 2 };
              const v3 radiance{ material->mEmissive };
              const v3 power{ radiance * area * 3.14f };
              const PatchPower patchPower
              {
                .mTriVerts           { triVerts },
                .mTotalPower         { power },
                .mCurrentUnshotPower { power },
                .mUnitNormal         { normal / normalLen },
                .mArea               { area },
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

    float ComputeTotalUnshotPower()
    {
      float total{};
      for( Instance& instance : mInstances )
        for( PatchPower& patchPower : instance.mPatchPowers )
          total += ( patchPower.mCurrentUnshotPower.x +
                     patchPower.mCurrentUnshotPower.y +
                     patchPower.mCurrentUnshotPower.z ) / 3;
      return total;
    }

    void Bake()
    {
      float minPowerLimit{ 0.01f };
      int maxIterations{ 100 };
      int samplesPerIteration = 1000;

      for( int iter{}; iter < maxIterations; ++iter )
      {
        const float totalUnshotPower{ ComputeTotalUnshotPower() };

        if( totalUnshotPower < minPowerLimit )
          break;

        TAC_NO_OP;

        for( Instance& instance : mInstances)
        {
          for( PatchPower& patchPower : instance.mPatchPowers)
          {
            const float currentUnshotPower{ ( patchPower.mCurrentUnshotPower.x +
                                              patchPower.mCurrentUnshotPower.y +
                                              patchPower.mCurrentUnshotPower.z ) / 3 };
            const float q_i{ currentUnshotPower / totalUnshotPower };
            const int N_i{ ( int )( q_i * samplesPerIteration ) };
            for( int iSample{}; iSample < N_i; ++iSample)
            {
              //          1
              // p(x) = -----
              //         A_i
              const v3 samplePoint{ RandomPointInTriangle( patchPower.mTriVerts[ 0 ],
                                                           patchPower.mTriVerts[ 1 ],
                                                           patchPower.mTriVerts[ 2 ] ) };

              //              cos(theta)
              //   p(omega) = ----------
              //                  pi
              const v3 sampleDir{ SampleCosineWeightedHemisphere( patchPower.mUnitNormal ) };

              //                  1     cos(theta)
              //   p(x,omega) = ----- * ----------
              //                 A_i        pi



            }
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
      sPreBakeScene.Bake();
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

#endif // TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
