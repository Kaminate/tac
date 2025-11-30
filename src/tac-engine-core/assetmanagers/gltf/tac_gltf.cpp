#include "tac_gltf.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac
{

  static auto FillDataTypePerElementByteCount( const cgltf_accessor* accessor ) -> int
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return 2;
      case cgltf_component_type_r_32f: return 4;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ); return 0;
    }
  }

  static auto FillDataTypePerElementDataType( const cgltf_accessor* accessor ) -> Render::GraphicsType
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return Render::GraphicsType::uint;
      case cgltf_component_type_r_32f: return Render::GraphicsType::real;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ); return ( Render::GraphicsType )0;
    }
  }

  static auto FillDataTypeElementCount( const cgltf_accessor* accessor ) -> int
  {
    switch( accessor->type )
    {
      case cgltf_type_scalar: return 1;
      case cgltf_type_vec2: return 2;
      case cgltf_type_vec3: return 3;
      case cgltf_type_vec4: return 4;
      default: TAC_ASSERT_INVALID_CASE( accessor->type ); return 0;
    }
  }

}

auto Tac::glTF_TypeToTac(cgltf_type gltfType) -> Tac::Render::VertexAttributeFormat
{
  switch( gltfType )
  {
    case cgltf_type_vec2: return Render::VertexAttributeFormat::GetVector2();
    case cgltf_type_vec3: return Render::VertexAttributeFormat::GetVector3();
    default: TAC_ASSERT_INVALID_CASE( gltfType ); return {};
  }
}

auto Tac::glTF_ComponentToTac( cgltf_component_type gltfType ) -> Tac::Render::TexFmt
{
  switch( gltfType )
  {
  //case cgltf_component_type_r_8u: return Render::TexFmt::kR8_uint;
  case cgltf_component_type_r_16u: return Render::TexFmt::kR16_uint;
  case cgltf_component_type_r_32u: return Render::TexFmt::kR32_uint;
  default: TAC_ASSERT_INVALID_CASE( gltfType ); return Render::TexFmt::kUnknown;
  }
}

auto Tac::glTF_ResultToString( const cgltf_result parseResult ) -> const char*
{
  const char* results[]
  {
    "cgltf_result_success",
    "cgltf_result_data_too_short",
    "cgltf_result_unknown_format",
    "cgltf_result_invalid_json",
    "cgltf_result_invalid_gltf",
    "cgltf_result_invalid_options",
    "cgltf_result_file_not_found",
    "cgltf_result_io_error",
    "cgltf_result_out_of_memory",
  };
  return results[ parseResult ];
}

auto Tac::glTF_AttributeToTac( cgltf_attribute_type attributeType ) -> Tac::Render::Attribute
{
  switch( attributeType )
  {
    case cgltf_attribute_type_position: return Render::Attribute::Position;
    case cgltf_attribute_type_normal:   return Render::Attribute::Normal;
    case cgltf_attribute_type_texcoord: return Render::Attribute::Texcoord;
    case cgltf_attribute_type_color:    return Render::Attribute::Color;
    case cgltf_attribute_type_joints:   return Render::Attribute::BoneIndex;
    case cgltf_attribute_type_weights:  return Render::Attribute::BoneWeight;
    case cgltf_attribute_type_tangent:  return Render::Attribute::Tangent;
    default: TAC_ASSERT_INVALID_CASE( attributeType ); return Render::Attribute::Unknown;
  }
}

auto Tac::glTF_AttributeFromTac( const Render::Attribute attributeType ) -> cgltf_attribute_type
{
  switch( attributeType )
  {
    case Render::Attribute::Position:   return cgltf_attribute_type_position;
    case Render::Attribute::Normal:     return cgltf_attribute_type_normal;
    case Render::Attribute::Texcoord:   return cgltf_attribute_type_texcoord;
    case Render::Attribute::Color:      return cgltf_attribute_type_color;
    case Render::Attribute::BoneIndex:  return cgltf_attribute_type_joints;
    case Render::Attribute::BoneWeight: return cgltf_attribute_type_weights;
    case Render::Attribute::Tangent:    return cgltf_attribute_type_tangent;
    default: TAC_ASSERT_INVALID_CASE( attributeType ); return cgltf_attribute_type_invalid;
  }
}

auto Tac::glTF_PrimitiveFromTac( const Render::PrimitiveTopology topology ) -> cgltf_primitive_type
{
  switch( topology )
  {
    case Render::PrimitiveTopology::TriangleList: return cgltf_primitive_type_triangles;
    default: TAC_ASSERT_INVALID_CASE( topology ); return {};
  }
}

auto Tac::glTF_CallHelper( const cgltf_result fnResult, const char* fnName, const char* fnArgs ) -> const char*
{

  const char* gltfErrorStr { glTF_ResultToString( fnResult ) };
  String msg;
  msg += fnName;
  msg += "( ";
  msg += fnArgs;
  msg += " )";
  msg += " returned ";
  msg += gltfErrorStr;
  return FrameMemoryCopy( msg );
}

auto Tac::glTF_AccessorToTac( const cgltf_accessor* accessor ) -> Tac::Render::VertexAttributeFormat
{
  return Render::VertexAttributeFormat
  {
    .mElementCount         { FillDataTypeElementCount( accessor ) },
    .mPerElementByteCount  { FillDataTypePerElementByteCount( accessor ) },
    .mPerElementDataType   { FillDataTypePerElementDataType( accessor ) },
  };
}

auto Tac::glTF_FindAttribute( const cgltf_primitive* prim, const cgltf_attribute_type type ) -> const cgltf_attribute*
{
  for( int iAttrib{}; iAttrib < ( int )prim->attributes_count; ++iAttrib )
    if( cgltf_attribute* gltfVertAttributeCurr{ &prim->attributes[ iAttrib ] };
        gltfVertAttributeCurr->type == type )
      return gltfVertAttributeCurr;
  return nullptr;
}

auto Tac::glTF_GetComponentMetaType( cgltf_component_type type ) -> const Tac::MetaType*
{
  switch( type )
  {
  case cgltf_component_type_r_8: return &GetMetaType< i8 >();
  case cgltf_component_type_r_8u: return &GetMetaType< u8 >();
  case cgltf_component_type_r_16: return &GetMetaType< i16 >();
  case cgltf_component_type_r_16u: return &GetMetaType< u16 >();
  case cgltf_component_type_r_32u: return &GetMetaType< u32 >();
  case cgltf_component_type_r_32f: return &GetMetaType< r32 >();
  default: TAC_ASSERT_INVALID_CASE( type ); return nullptr;
  }
}

