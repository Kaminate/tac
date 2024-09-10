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
    static dynmc Material*        GetMaterial( dynmc Entity* );
    static const Material*        GetMaterial( const Entity* );
    const ComponentInfo*          GetEntry() const override;

    static void                   RegisterComponent();
    static void                   DebugImgui( Material* );

    // ...

    struct Data
    {
      String mKey   {};
      Json   mValue {};
    };


    // Collection of data of various material shaders.
    // The actual material shader picks which data to use
    bool            mIsGlTF_PBR_MetallicRoughness  {};
    bool            mIsGlTF_PBR_SpecularGlossiness {};
    v4              mColor                         {};
    v3              mEmissive                      {};

    // this could be used if we were to support arbitrary data
    Vector< Data >  mData           {};

    AssetPathString mShaderGraph;

    // Whether entities with this material component should render or not
    bool            mRenderEnabled  { true };
  };

  TAC_META_DECL( Material );

} // namespace Tac

