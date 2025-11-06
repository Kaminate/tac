#include "tac_iapp.h" // self-inc

namespace Tac
{
  static App* sInstance;

  App::App( const Config& config ) : mConfig( config ) { sInstance = this;  }
  bool App::IsRenderEnabled() const               { return !mConfig.mDisableRenderer; }
  auto App::GetAppName() const -> StringView      { return mConfig.mName; }
  auto App::GetStudioName() const -> StringView   { return mConfig.mStudioName; }
  auto App::GameState_Create() -> State           { return { TAC_NEW IState }; } // dummy state
  auto App::Instance() -> App*                    { return sInstance; }
} // namespace Tac

