#pragma once

namespace Tac
{
  struct StringView;
  bool ImGuiDragFloatN( const StringView&, float*, int );
  bool ImGuiDragIntN( const StringView&, int*, int );
} // namespace Tac
