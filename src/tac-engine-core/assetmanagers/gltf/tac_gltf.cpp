#include "tac_gltf.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"

Tac::Render::VertexAttributeFormat Tac::GLTFTypeToTacVertexAttributeFormat(cgltf_type gltfType)
{
  switch( gltfType )
  {
    case cgltf_type_vec2: return Render::VertexAttributeFormat::GetVector2();
    case cgltf_type_vec3: return Render::VertexAttributeFormat::GetVector3();
    default: TAC_ASSERT_INVALID_CASE( gltfType ); return {};
  }
}

Tac::Render::TexFmt Tac::ConvertIndexFormat( cgltf_component_type gltfType )
{
  switch( gltfType )
  {
  //case cgltf_component_type_r_8u: return Render::TexFmt::kR8_uint;
  case cgltf_component_type_r_16u: return Render::TexFmt::kR16_uint;
  case cgltf_component_type_r_32u: return Render::TexFmt::kR32_uint;
  default: TAC_ASSERT_INVALID_CASE( gltfType ); return Render::TexFmt::kUnknown;
  }
}

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  static int                  FillDataTypePerElementByteCount( const cgltf_accessor* accessor )
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return 2;
      case cgltf_component_type_r_32f: return 4;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ); return 0;
    }
  }

  static Render::GraphicsType FillDataTypePerElementDataType( const cgltf_accessor* accessor )
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return Render::GraphicsType::uint;
      case cgltf_component_type_r_32f: return Render::GraphicsType::real;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ); return ( Render::GraphicsType )0;
    }
  }

  static int                  FillDataTypeElementCount( const cgltf_accessor* accessor )
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



  // -----------------------------------------------------------------------------------------------


  const char*                 GetcgltfErrorAsString( const cgltf_result parseResult )
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

  Render::Attribute        GLTFToTacAttribute( cgltf_attribute_type attributeType )
  {
    switch( attributeType )
    {
      case cgltf_attribute_type_position: return Render::Attribute::Position;
      case cgltf_attribute_type_normal:   return Render::Attribute::Normal;
      case cgltf_attribute_type_texcoord: return Render::Attribute::Texcoord;
      case cgltf_attribute_type_color:    return Render::Attribute::Color;
      case cgltf_attribute_type_joints:   return Render::Attribute::BoneIndex;
      case cgltf_attribute_type_weights:  return Render::Attribute::BoneWeight;

      default: TAC_ASSERT_INVALID_CASE( attributeType ); return Render::Attribute::Unknown;
    }
  }

  cgltf_attribute_type        GetGltfFromAttribute( const Render::Attribute attributeType )
  {
    switch( attributeType )
    {
      case Render::Attribute::Position:   return cgltf_attribute_type_position;
      case Render::Attribute::Normal:     return cgltf_attribute_type_normal;
      case Render::Attribute::Texcoord:   return cgltf_attribute_type_texcoord;
      case Render::Attribute::Color:      return cgltf_attribute_type_color;
      case Render::Attribute::BoneIndex:  return cgltf_attribute_type_joints;
      case Render::Attribute::BoneWeight: return cgltf_attribute_type_weights;

      default: TAC_ASSERT_INVALID_CASE( attributeType ); return cgltf_attribute_type_invalid;
    }
  }

  cgltf_primitive_type        GetGltfFromTopology( const Render::PrimitiveTopology topology )
  {
    switch( topology )
    {
      case Render::PrimitiveTopology::TriangleList: return cgltf_primitive_type_triangles;
      default: TAC_ASSERT_INVALID_CASE( topology ); return {};
    }
  }

  const char*                 GltfFmtErrMsg( const cgltf_result fnResult,
                                    const char* fnName,
                                    const char* fnArgs )
  {

    const char* gltfErrorStr { GetcgltfErrorAsString( fnResult ) };

    String msg;
    msg += fnName;
    msg += "( ";
    msg += fnArgs;
    msg += " )";
    msg += " returned ";
    msg += gltfErrorStr;

    return FrameMemoryCopy( msg );
  }

  Render::VertexAttributeFormat FillDataType( const cgltf_accessor* accessor )
  {
    return Render::VertexAttributeFormat
    {
      .mElementCount         { FillDataTypeElementCount( accessor ) },
      .mPerElementByteCount  { FillDataTypePerElementByteCount( accessor ) },
      .mPerElementDataType   { FillDataTypePerElementDataType( accessor ) },
    };
  }


  const cgltf_attribute*      FindAttributeOfType( const cgltf_primitive* parsedPrim,
                                                  const cgltf_attribute_type type )
  {
    for( int iAttrib {}; iAttrib < ( int )parsedPrim->attributes_count; ++iAttrib )
      if( cgltf_attribute* gltfVertAttributeCurr { &parsedPrim->attributes[ iAttrib ] };
          gltfVertAttributeCurr->type == type )
        return gltfVertAttributeCurr;
    return nullptr;
  }

  const MetaType*               FindMetaType_from_cgltf_component_type( cgltf_component_type type )
  {
    switch( type )
    {
    case cgltf_component_type_r_8: return &GetMetaType< i8 >();
    case cgltf_component_type_r_8u: return &GetMetaType< u8 >();
    case cgltf_component_type_r_16: return &GetMetaType< i16 >();
    case cgltf_component_type_r_16u:return  &GetMetaType< u16 >();
    case cgltf_component_type_r_32u: return &GetMetaType< u32 >();
    case cgltf_component_type_r_32f: return &GetMetaType< r32 >();
    default: TAC_ASSERT_INVALID_CASE( type ); return nullptr;
    }
  }

} // namespace Tac
