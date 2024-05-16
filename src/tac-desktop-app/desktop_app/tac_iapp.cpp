#include "tac_iapp.h" // self-inc

namespace Tac
{
  App::IState* App::GetGameState()          { return nullptr; }
  bool         App::IsRenderEnabled() const { return !mConfig.mDisableRenderer; }
  StringView   App::GetAppName() const      { return mConfig.mName; }
  StringView   App::GetStudioName() const   { return mConfig.mStudioName; }
} // namespace Tac

