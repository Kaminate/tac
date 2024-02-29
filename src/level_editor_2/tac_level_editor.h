#pragma once


namespace Tac
{
  struct Errors;

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );

    bool mShowUnownedWindow{};
    bool mShowOwnedWindow{};
  };

  extern Creation gCreation;

} // namespace Tac

