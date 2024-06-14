#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  String HLSLPreprocess( AssetPathStringView, Errors& );
} // namespace Tac::Render

