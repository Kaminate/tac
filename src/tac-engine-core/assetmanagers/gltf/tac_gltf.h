#pragma once

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"

#if _MSC_VER
#pragma warning( disable : 26451 )
#pragma warning( disable : 26819 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 6001 )
#pragma warning( disable : 6262 )
#pragma warning( disable : 6308 )
#endif

#include "tac-engine-core/thirdparty/cgltf.h"

namespace Tac
{
  cgltf_attribute_type          GetGltfFromAttribute( Render::Attribute );
  cgltf_primitive_type          GetGltfFromTopology( Render::PrimitiveTopology );
  Render::Attribute             GLTFToTacAttribute( cgltf_attribute_type );

  const char*                   GetcgltfErrorAsString( cgltf_result );
  const char*                   GltfFmtErrMsg( cgltf_result, const char*, const char* );

  Render::VertexAttributeFormat FillDataType( const cgltf_accessor* );
  const cgltf_attribute*        FindAttributeOfType( const cgltf_primitive*,
                                                     cgltf_attribute_type );
  

} // namespace Tac

#define TAC_GLTF_CALL( fn, ... ) {                     \
  const cgltf_result fnResult = fn( __VA_ARGS__ );             \
  if( fnResult != cgltf_result_success )                       \
  {                                                            \
      const char* msg = Tac::GltfFmtErrMsg( fnResult,          \
                                            #fn,               \
                                            #__VA_ARGS__ );    \
      TAC_RAISE_ERROR( msg );                          \
  }                                                            \
}
