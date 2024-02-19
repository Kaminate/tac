#pragma once

namespace Tac
{
  struct Errors;
  struct App;

  struct LogicThread
  {
    void Init( Errors& );
    void Update( Errors& );
    void Uninit();

    App*    mApp = nullptr;
    Errors* mErrors = nullptr;
  };

}
