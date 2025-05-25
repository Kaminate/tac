#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/meta/tac_meta_impl.h"
#include "tac-engine-core/asset/tac_asset.h"

namespace Tac
{
  struct Material : public Component
  {
    const ComponentInfo* GetEntry() const override;

    static auto GetMaterial( dynmc Entity* ) -> dynmc Material*;
    static auto GetMaterial( const Entity* ) -> const Material*;
    static void RegisterComponent();
    static void DebugImgui( Material* );

    // this `Data` struct could just be a `Json`
    struct Data
    {
      String mKey   {};
      Json   mValue {};
    };

    // Collection of data of various material shaders.
    // The actual material shader picks which data to use
    bool            mIsGlTF_PBR_MetallicRoughness  {};
    bool            mIsGlTF_PBR_SpecularGlossiness {};
    float           mPBR_Factor_Metallic           {};
    float           mPBR_Factor_Roughness          {};
    v3              mPBR_Factor_Diffuse            {};
    v3              mPBR_Factor_Specular           {};
    float           mPBR_Factor_Glossiness         {};
    v4              mColor                         {}; // units?
    v3              mEmissive                      {}; // units?

    struct LightEmissionRuntime
    {
      // Diffuse area light

      struct RGB
      {
        float r;
        float g;
        float b;
      };

      struct RGBColorSpace
      {

      };

#if 0
      // used in DiffuseAreaLight::L if its an texture mapped
      struct RGBIlluminantSpectrum
      {
        RGBIlluminantSpectrum() = default;
        RGBIlluminantSpectrum(RGBColorSpace cs, RGB rgb) = default;
        ... mValues
          mScale; // scales the rgb when computing a sigmoid, reusused later?
        ;
      };
#endif

      struct SampledWavelengths // spectrum.h
      {
        float mLambdas[4];
        float mPDFs[4];
      };

      struct SampledSpectrum // spectrum.h
      {
        float mValues[4];
      };

      static constexpr float Lambda_min = 360;
      static constexpr float Lambda_max = 830;

      struct DenselySampledSpectrum
      {
        float mValues[ (int)Lambda_max - (int)Lambda_min + 1] {};
      };

      // radiant flux / radiant exitance (watts), symbol Phi
      SampledSpectrum Phi( SampledWavelengths lambda )
      {
        // E(p) = \frac{d\Phi(p)}{dA}
        //
        // (4.1)
        // \Phi = \int_A E(p) dA
        //
        // \Phi = A * E()
        // \Phi = A * \int L(\omega) cos(\theta) d\omega
        // \Phi = A * mSpectrum * \int cos(\theta) d\omega
        // \Phi = A * mSpectrum * \pi

        return SampledSpectrum{ 3.14f * mArea * mSpectrum.mValues };
      }


      DenselySampledSpectrum mSpectrum {};
      float       mArea     {};
    };

    struct LightEmissionFileSpecification
    {
      // radiometric  | unit | sym |      | photometric   | unit
      // -------------+------+-----+      +---------------+-----
      // Radiant flux | watt | phi |      | Luminous flux | lumen

      // Example 1ft x 4ft panel light on amazon.com has 4000 lumens
      float mLumens = 0;
    };

    AssetPathString mTextureDiffuse                {};
    AssetPathString mTextureSpecular               {};
    AssetPathString mTextureGlossiness             {};
    AssetPathString mTextureMetallic               {};
    AssetPathString mTextureRoughness              {};

    // this could be used if we were to support arbitrary data
    Vector< Data >  mData                          {};

    AssetPathString mShaderGraph;

    // Whether entities with this material component should render or not
    bool            mRenderEnabled                 { true };
  };

  TAC_META_DECL( Material );

} // namespace Tac

