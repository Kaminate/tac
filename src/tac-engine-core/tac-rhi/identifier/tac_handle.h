#pragma once

namespace Tac
{
  struct Handle
  {
    Handle( int index = -1 );

    explicit operator int() const;
    explicit operator unsigned() const;

    bool operator ==( Handle ) const;
    bool operator !=( Handle ) const;
    bool              IsValid() const;
    int               GetIndex() const;

  private:
    int               mIndex;
  };

#define TAC_DEFINE_HANDLE( T )      \
  struct T : public Handle          \
  {                                 \
    T( int i = -1 ) : Handle( i ){} \
  };


} // namespace Tac

