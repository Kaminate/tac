#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"

namespace Tac
{
  using MeshLoadFunction = Mesh( * )( ModelAssetManager::Params, Errors& );
  using MeshFileExt = StringView;

  void ModelLoadFunctionRegister( MeshLoadFunction, MeshFileExt );
  auto ModelLoadFunctionFind( MeshFileExt ) -> MeshLoadFunction;
}

