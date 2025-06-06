#include "tac_radiosity_baker.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-std-lib/os/tac_os.h"

 #if TAC_SHOULD_IMPORT_STD()
   import std;
 #else
   #include <limits>
 #endif

namespace Tac
{
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

  auto PreBakeScene::Raycast( PatchPower* fromPatch, RayTriangle::Ray ray ) -> PreBakeScene::RaycastResult
  {
    constexpr float kFloatMax{ std::numeric_limits<float>::max() };
    dynmc RaycastResult result{ .mT { kFloatMax } };

    for( Instance& instance : mInstances )
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
        if( !raycastResult.mValid || raycastResult.mT >= result.mT )
          continue;

        result.mHitPatch = &patchPower;
        result.mHitPatchMaterial = instance.mMaterial;
        result.mT = raycastResult.mT;
      }
    }

    return result;
  }

  auto PreBakeScene::GetMesh(const Model* model) -> const Mesh*
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

  auto PreBakeScene::PatchPower::GetUnshotPower() const -> float
  {
    // When you look at this function signature, you may think:
    //
    // Q: How is it a valid operation to turn radiometric power, which irl is a spectrum,
    //    into a single floating point return value?
    //
    // A: As you may be aware, in the radiosity baker program, a spectrum is represented as a
    //    v3 linear srgb, representing a weighted sum of the primary color spectrums.
    //
    //    This function returns the average value of that, which is sort of a heuristic, 
    //    but the important thing is that the return value is used to sample 1 light among many,
    //    and get a probability for choosing that particular light.
    //
    //    A similar thing in PBRT may be the PowerLightSampler and SampledLight 
    //    https://pbr-book.org/4ed/Light_Sources/Light_Sampling
    return ( mCurrentUnshotPower.x + mCurrentUnshotPower.y + mCurrentUnshotPower.z ) / 3;
  }

  auto PreBakeScene::PatchPower::GetRandomSurfacePoint() const -> v3
  {
    return RandomPointInTriangle( mTriVerts[ 0 ], mTriVerts[ 1 ], mTriVerts[ 2 ] );
  }

  void PreBakeScene::Init(const World* world)
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
        for( JPPTCPUMeshData::IndexType i : jpptCPUMeshData.mIndexes )
        {
          const v3& vert{ jpptCPUMeshData.mPositions[ i ] };
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

  auto PreBakeScene::ComputeTotalUnshotPower() -> float
  {
    float total{};
    for( Instance& instance : mInstances )
      for( PatchPower& patchPower : instance.mPatchPowers )
        total += patchPower.GetUnshotPower();
    return total;
  }

  void PreBakeScene::Bake()
  {
    float minPowerLimit{ 0.01f };
    int maxJacobiIterations{ 100 };
    int samplesPerIteration = 1000;

    for( int iJacobi{}; iJacobi < maxJacobiIterations; ++iJacobi )
    {
      const float totalUnshotPower{ ComputeTotalUnshotPower() };

      if( totalUnshotPower < minPowerLimit )
        break;

      TAC_NO_OP;

      for( Instance& instance : mInstances)
      {
        for( PatchPower& patchPower_src : instance.mPatchPowers)
        {
          // \Delta P_{src}^{(k)}
          const float unshotPower_src{ patchPower_src.GetUnshotPower() };

          // Probability of sampling this light source
          //
          //          \Delta P_src^{(k)}
          // p(src) = ------------------
          //           \Delta P_T
          const float q_src{ unshotPower_src / totalUnshotPower };

          // Number of samples allocated to this light source
          const int N_src{ ( int )( q_src * samplesPerIteration ) };

          for( int sample_src_index{}; sample_src_index < N_src; ++sample_src_index)
          {
            //          1
            // p(x) = -----
            //        A_src
            const v3 samplePoint_src_worldspace{ patchPower_src.GetRandomSurfacePoint() };

            //              cos(theta)
            //   p(omega) = ----------
            //                  pi
            const v3 sampleDir_src_worldspace{ SampleCosineWeightedHemisphere( patchPower_src.mUnitNormal ) };

            //
            // p( src, dst ) = p( src ) * p( dst | src )
            //                 |          |
            //     +-----------+          +-------------------------------------+
            //     |                                                            |
            //     +-> Probability p( src ) of sampling the src patch           |
            //                                                                  |
            //                    \Delta P_src^{(k)}                            |
            //         p( src ) = ------------------                            |
            //                        \Delta P_T                                |
            //                                                                  |
            //     +------------------------------------------------------------+
            //     |          
            //     +-> Probability p( dst ) of sampling the dst patch
            //      
            //         p( dst ) = F_{src, dst}
            //                    |
            //         +----------+
            //         |
            //         +-> Form factor F_{src, dst} describing power emitted by src, received by dst
            //
            //             F_{src, dst} = P_{src, dst}
            //                            |
            //             +--------------+
            //             |
            //             +-> Probability P_{src, dst} of ray originating on patch src landing on patch dst
            //
            //                 P_{src, dst} = \int_{S_{src}} \int_{\Omega_x} \chi_{dst}(x, \Omega) p(x,\Omega) dA_x d \omega_\Omega
            //                                                               |                     |
            //                 +---------------------------------------------+                     |
            //                 |                                                                   |
            //                 +-> 1 or 0 if a ray shot from src at point x into \Omega hits dst   |
            //                                                                                     |
            //                     \chi_{dst}(x, \Omega) = { 1 if the ray hits }                   |
            //                                             { 0 otherwise }                         |
            //                                                                                     |
            //                 +-------------------------------------------------------------------+
            //                 |
            //                 +-> Probability density p(x,omega) of ray
            //                                
            //                     p(x,omega) = p(x) * p(omega)
            //                                  |      |
            //                     +------------+      +-----------------------------+
            //                     |                                                 |
            //                     +-> Probability of x uniformly chosen on src      |
            //                                                                       |
            //                                  1                                    |
            //                         p(x) = -----                                  |
            //                                A_src                                  |
            //                                                                       |
            //                     +-------------------------------------------------+
            //                     |
            //                     +-> Probability p(omega) ray in direction omega, cos weighted
            //
            //                                    cos(theta)
            //                         p(omega) = ----------
            //                                        pi

            // todo: bounce the light around the scene
            const RayTriangle::Ray ray_src_to_dst
            {
              .mOrigin    { samplePoint_src_worldspace },
              .mDirection { sampleDir_src_worldspace },
            };
            const RaycastResult raycastResult{ Raycast( &patchPower_src, ray_src_to_dst ) };
            if( !raycastResult.mHitPatch )
              continue; // \delta_{li}

            PatchPower& patchPower_dst { *raycastResult.mHitPatch };
            const v3 rho_dst { raycastResult.mHitPatchMaterial->mColor.xyz() };

            // [ ] Q: should raycastResult.mHitPatchMaterial->mColor.xyz() be converted from encded srgb to linear srgb?
            const v3 sample_i
            {
              rho_dst *
              ( 1.0f / samplesPerIteration ) * // ??? averaging a bunch of samples ???
              totalUnshotPower // \Delta P_T^{(k)}
            }; 
            patchPower_dst.mCurrentReceivedPower += sample_i;


            /*
               -----------------------------------------------------------------------------
                                          The Monte Carlo Method
               -----------------------------------------------------------------------------
            
               Say we want to compute the sum S of std::vector<int> numbers { 10, 13, 43, 74, 25 }
            
                 S = \sum_{i=1}^{n} a_i
            
               In the monte carlo method, an "estimator" is a random number whose "expectation"
               is equal to the value we are trying to compute. The estimator we will use is:
            
                 \hat{S} = (n a_i, \frac{1}{n})

                 which has value n a_i and probability \frac{1}{n}
            
               This is what it looks like in code:
            
                 #include <vector>
                 #include <iostream>
                 #include <numeric>
                 int main()
                 {
                   std::vector<int> numbers { 10, 13, 43, 74, 25 };
                   std::cout << "actual sum: " << std::accumulate( numbers.begin(), numbers.end(), 0 ) << std::endl;
                   int a_i = numbers[ std::rand() % numbers.size() ];
                   int S = numbers.size() * a_i;
                   std::cout << "monte carlo sum: " << S << std::endl;
                 }
            
               Only the estimator value is used in computing the sum, but it is important that the
               samples are chosen with the specified probability \frac{1}{n}.

               Now this gives kind of a shitty estimate of the sum, but with a monte carlo estimator
               it is kind of assumed that we reduce the variance by averaging multiple samples.

                 #include <vector>
                 #include <iostream>
                 #include <numeric>
                 int main()
                 {
                   std::vector<int> numbers { 10, 13, 43, 74, 25 };
                   std::cout << "actual sum: " << std::accumulate( numbers.begin(), numbers.end(), 0 ) << std::endl;
                   int N = 10000;
                   float S = 0;
                   for( int i = 0; i < N; ++i )
                   {
                     int a_i = numbers[ std::rand() % numbers.size() ];
                     S += numbers.size() * a_i / (float)N;
                   }
                   std::cout << "monte carlo sum: " << S << std::endl;
                 }
            
            */

            /*
               The value we would like to estimate is:
              
                 \Delta P_i^{k+1} = \sum_j \sum_{l \new j } \Delta P_j^{(k)}F_{jl}\rho_l \delta_{li}
              
               So we will use the following estimator
              
                 \Delta \hat{P_i} = ( \rho_i \Delta P_T^{(k)} \delta_{li}, \frac{\Delta P_j^{(k)}}{\Delta P_T^{(k)}} F_{jl} )
              
               In code, what that means is the sample value of \rho_i \Delta P_T^{(k)} \delta_{li}
               is itself an estimate for the i'th PatchPower::mCurrentReceivedPower.

               To reiterate, only the estimator value is used to compute \Delta P_i^{k+1}
               but it is important that the samples are chosen with the specified probabilty.
               \frac{\Delta P_j^{(k)}}{\Delta P_T^{(k)}} F_{jl}
            */


            if( false )
            {
              mDebugLine = true;
              mDebugSrcPos = ray_src_to_dst.mOrigin;
              mDebugDstPos = ray_src_to_dst.mOrigin + ray_src_to_dst.mDirection * raycastResult.mT;
              mDebugSrcPatch = &patchPower_src;
              mDebugDstPatch = &patchPower_dst;
              return;
            }


          } // for each sample
        } // for each patch
      } // for each instance

      for( Instance& instance : mInstances )
      {
        for( PatchPower& patchPower : instance.mPatchPowers)
        {
          patchPower.mTotalPower += patchPower.mCurrentReceivedPower;
          patchPower.mCurrentUnshotPower = patchPower.mCurrentReceivedPower;
          patchPower.mCurrentReceivedPower = {};
        }
      }

    } // for each jacobi iteration
  } // void PreBakeScene::Bake()



} // namespace Tac

