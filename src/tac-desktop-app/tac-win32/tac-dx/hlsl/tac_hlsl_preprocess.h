#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct HLSLPreprocessor
  {
    static String Process( Vector< AssetPathString >, Errors& );
  };
} // namespace Tac::Render

