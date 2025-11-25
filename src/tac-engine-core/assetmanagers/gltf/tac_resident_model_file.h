// Q: WTF is the difference between:
//
// - tac_resident_model_file.h
// - tac_model_load_synchronous.h
// - tac_model_asset_manager.h

#pragma once

struct cgltf_data;

namespace Tac
{
  struct AssetPathStringView;

  auto TryGetGLTFData( const AssetPathStringView& ) -> const cgltf_data*;
}

