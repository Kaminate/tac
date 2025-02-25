#include "tac_math_meta.h" // self-inc

#include "tac-std-lib/meta/tac_meta_impl.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_matrix4.h"

namespace Tac
{

  TAC_META_REGISTER_COMPOSITE_BEGIN( v2 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( x );
  TAC_META_REGISTER_COMPOSITE_MEMBER( y );
  TAC_META_REGISTER_COMPOSITE_END( v2 );

  TAC_META_REGISTER_COMPOSITE_BEGIN( v3 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( x );
  TAC_META_REGISTER_COMPOSITE_MEMBER( y );
  TAC_META_REGISTER_COMPOSITE_MEMBER( z );
  TAC_META_REGISTER_COMPOSITE_END( v3 );

  TAC_META_REGISTER_COMPOSITE_BEGIN( v4 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( x );
  TAC_META_REGISTER_COMPOSITE_MEMBER( y );
  TAC_META_REGISTER_COMPOSITE_MEMBER( z );
  TAC_META_REGISTER_COMPOSITE_MEMBER( w );
  TAC_META_REGISTER_COMPOSITE_END( v4 );

  TAC_META_REGISTER_COMPOSITE_BEGIN( m3 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m00 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m01 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m02 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m10 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m11 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m12 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m20 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m21 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m22 );
  TAC_META_REGISTER_COMPOSITE_END( m3 );

  TAC_META_REGISTER_COMPOSITE_BEGIN( m4 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m00 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m01 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m02 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m03 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m10 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m11 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m12 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m13 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m20 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m21 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m22 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m23 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m30 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m31 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m32 );
  TAC_META_REGISTER_COMPOSITE_MEMBER( m33 );
  TAC_META_REGISTER_COMPOSITE_END( m4 );

} // namespace Tac

