#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  // The MaterialInput describes ... $$$ todo comment
  struct MaterialInput
  {
    using UnderlyingType = u32;

    // $$$ instead of creating a custom constant buffer type, just fucking make a uberbuffer
    enum class Type : UnderlyingType
    {
      kUnknown      = 0,
      kWorldMatrix  = 1 << 0,
      kVertexBuffer = 1 << 1,
      kInputLayout  = 1 << 2,
    };

    static constexpr Type kFirstType { Type::kWorldMatrix };
    static constexpr Type kLastType  { Type::kInputLayout };

    struct TypeIterator
    {
      TypeIterator begin() const;
      TypeIterator end() const;
      Type         operator *() const;
      void         operator ++();
      bool         operator == ( const TypeIterator& ) const = default;
      Type         mType;
    };


    static StringView    Type_to_String( Type );
    static Type          String_to_Type( StringView );

    static MaterialInput JsonLoad( const Json* );
    static Json          JsonSave( const MaterialInput& );

    void Set( Type );
    void Set( Type, bool );
    void Clear( Type );
    void Clear();
    bool IsSet( Type ) const;

  private:

    Type mBitfield{};
  };

} // namespace Tac

