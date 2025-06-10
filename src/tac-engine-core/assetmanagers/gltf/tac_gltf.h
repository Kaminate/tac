#pragma once

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/meta/tac_meta.h"

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

#include "tac-engine-core/thirdparty/cgltf/cgltf.h"
#include "tac-engine-core/thirdparty/cgltf/cgltf_write.h"

namespace Tac
{
  auto glTF_AttributeFromTac( Render::Attribute ) -> cgltf_attribute_type;
  auto glTF_PrimitiveFromTac( Render::PrimitiveTopology ) -> cgltf_primitive_type;
  auto glTF_AttributeToTac( cgltf_attribute_type ) -> Render::Attribute;
  auto glTF_ComponentToTac( cgltf_component_type ) -> Render::TexFmt;
  auto glTF_ResultToString( cgltf_result ) -> const char*;
  auto glTF_AccessorToTac( const cgltf_accessor* ) -> Render::VertexAttributeFormat;
  auto glTF_FindAttribute( const cgltf_primitive*, cgltf_attribute_type ) -> const cgltf_attribute*;
  auto glTF_GetComponentMetaType( cgltf_component_type ) -> const MetaType*;
  auto glTF_TypeToTac( cgltf_type ) -> Render::VertexAttributeFormat;
  auto glTF_CallHelper( cgltf_result, const char*, const char* ) -> const char*;

} // namespace Tac

#define TAC_GLTF_CALL( fn, ... ) {                             \
  const cgltf_result fnResult{ fn( __VA_ARGS__ ) };            \
  if( fnResult != cgltf_result_success )                       \
  {                                                            \
      const char* msg{ Tac::glTF_CallHelper( fnResult,         \
                                            #fn,               \
                                            #__VA_ARGS__ ) };  \
      TAC_RAISE_ERROR( msg );                                  \
  }                                                            \
}
