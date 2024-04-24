#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac
{
  struct WindowCreateParams
  {
    StringView  mName { "" };
    v2i         mPos;
    v2i         mSize;
  };
} // namespace Tac


