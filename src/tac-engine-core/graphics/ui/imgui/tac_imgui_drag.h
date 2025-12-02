#pragma once
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  bool ImGuiDragFloatN( StringView, float*, int );
  bool ImGuiDragIntN( StringView, int*, int );
} // namespace Tac
