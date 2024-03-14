#pragma once

namespace Tac { struct String; struct Errors; struct AssetPathStringView; }
namespace Tac::Render
{
  String DX12PreprocessShader( const AssetPathStringView&, Errors& );
} // namespace Tac::Render

