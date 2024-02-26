#pragma once


namespace Tac
{
  struct Errors;

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );

    bool mShowMainWindow{};
  };

  extern Creation gCreation;

} // namespace Tac

