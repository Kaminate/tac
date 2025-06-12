#include "tac_font.h" // self-inc
#include "tac_font_backend.h"

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/thirdparty/stb/stb_truetype.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/memory/tac_memory_util.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"


#if TAC_FONT_ENABLED()

namespace Tac
{


  // -----------------------------------------------------------------------------------------------

  void                  FontApi::Init( Errors& errors )
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    fontAtlas.Load( errors );
  }

  void                  FontApi::Uninit()
  {
    //FontAtlas& fontAtlas{ FontAtlas::Instance };
    FontAtlas::Instance.Uninit();
  }

  const FontDims*       FontApi::GetLanguageFontDims( Language language )
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.GetLanguageFontDims( language );
  }

  const FontAtlasCell*  FontApi::GetFontAtlasCell( Language language,
                                                   Codepoint codepoint )
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.GetCharacter( language, codepoint );
  }

  Render::TextureHandle FontApi::GetAtlasTextureHandle()
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.GetTextureHandle();
  }

  float                 FontApi::GetSDFOnEdgeValue()
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.GetSDFOnEdgeValue();
  }

  float                 FontApi::GetSDFPixelDistScale()
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.GetSDFPixelDistScale();
  }

  void                  FontApi::UpdateGPU(Errors&errors)
  {
    FontAtlas& fontAtlas{ FontAtlas::Instance };
    return fontAtlas.UpdateGPU(errors);
  }

}
#endif
