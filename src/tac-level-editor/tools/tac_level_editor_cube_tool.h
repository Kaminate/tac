#pragma once

#include "tac-level-editor/tools/tac_level_editor_tool.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct CubeTool : public Tool
  {
    CubeTool();
    void OnToolSelected() override;
    void Update() override;
    static CubeTool sInstance;
  };
}

