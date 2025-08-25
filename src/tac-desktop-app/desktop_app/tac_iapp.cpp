#include "tac_iapp.h" // self-inc

namespace Tac
{
  static App* sInstance;

  //App::IState* App::GetGameState()          { return nullptr; }
  bool         App::IsRenderEnabled() const { return !mConfig.mDisableRenderer; }
  StringView   App::GetAppName() const      { return mConfig.mName; }
  StringView   App::GetStudioName() const   { return mConfig.mStudioName; }

  App::App( const Config& config ) : mConfig( config ) { sInstance = this;  }
  auto App::Instance() -> App* { return sInstance; }
} // namespace Tac

