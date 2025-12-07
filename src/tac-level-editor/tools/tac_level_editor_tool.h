#pragma once

#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct Tool
  {
    virtual void OnToolSelected() {}
    virtual void Update() = 0;

    const char*     mDisplayName {};
    AssetPathString mIcon        {};
  };


  struct Toolbox
  {
    static void Init();
    static void SelectTool( Tool* );
    static void DebugImGui(Errors&);
  };
}

