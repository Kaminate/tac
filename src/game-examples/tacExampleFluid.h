#pragma once

namespace Tac
{

  struct Errors;
  struct ExampleFluid
  {
    void Init( const Errors& );
    void Update( const Errors& );
    void Uninit( const Errors& );
  };

}
