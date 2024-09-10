#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  struct MaterialInput
  {
    using UnderlyingType = u32;

    // $$$ instead of creating a custom constant buffer type, just fucking make a uberbuffer
    enum Type : UnderlyingType
    {
      kWorldMatrix,
      kVertexBuffer,
      kCount,
    };

    static MaterialInput FromJson( const Json* );
    static Json          ToJson( const MaterialInput& );

    void Set( Type t );
    void Set( Type t, bool b );
    void Clear( Type t );
    void Clear();
    bool IsSet( Type t ) const;

  private:

    Type mBitfield{};
  };

} // namespace Tac

