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

    // units? probably albedo, as a spectrum of reflectance values in some rgb color space.
    v4              mColor                         {};

    // If this material is emissive, then it acts as diffuse area light.
    // The emissive member variable reprements output radiance
    // as a spectrum of light using linear (non-encoded)
    // coefficients of the scRGB color space.
    v3              mEmissive                      {};


#if 0
    struct EmissionParams
    {
      float mIlluminanceInLumens {};
      float mTemperatureInKelvin {};
    };

    void SetEmission( EmissionParams params );
#endif


    AssetPathString mTextureDiffuse                {};
    AssetPathString mTextureSpecular               {};
    AssetPathString mTextureGlossiness             {};
    AssetPathString mTextureMetallic               {};
    AssetPathString mTextureRoughness              {};

    // this could be used if we were to support arbitrary data
    Vector< Data >  mData                          {};

    AssetPathString mShaderGraph                   {};

    // Whether entities with this material component should render or not
    bool            mRenderEnabled                 { true };
  };

  TAC_META_DECL( Material );

} // namespace Tac

