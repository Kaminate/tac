#include "tac_gltf.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"

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
    const char* results[] =
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

  cgltf_attribute_type        GetGltfFromAttribute( const Render::Attribute attributeType )
  {
    switch( attributeType )
    {
      case Render::Attribute::Position: return cgltf_attribute_type_position;
      case Render::Attribute::Normal: return cgltf_attribute_type_normal;
      case Render::Attribute::Texcoord: return cgltf_attribute_type_texcoord;
      case Render::Attribute::Color: return cgltf_attribute_type_color;
      case Render::Attribute::BoneIndex: return cgltf_attribute_type_joints;
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

    const char* gltfErrorStr = GetcgltfErrorAsString( fnResult );

    String msg;
    msg += fnName;
    msg += "( ";
    msg += fnArgs;
    msg += " )";
    msg += " returned ";
    msg += gltfErrorStr;

    return FrameMemoryCopy( msg );
  }


  Render::Format              FillDataType( const cgltf_accessor* accessor )
  {
    const Render::Format result =
    {
      .mElementCount = FillDataTypeElementCount( accessor ),
      .mPerElementByteCount = FillDataTypePerElementByteCount( accessor ),
      .mPerElementDataType = FillDataTypePerElementDataType( accessor ),
    };
    return result;
  }

  const cgltf_attribute*      FindAttributeOfType( const cgltf_primitive* parsedPrim,
                                                  const cgltf_attribute_type type )
  {
    for( int iAttrib = 0; iAttrib < ( int )parsedPrim->attributes_count; ++iAttrib )
    {
      cgltf_attribute* gltfVertAttributeCurr = &parsedPrim->attributes[ iAttrib ];
      if( gltfVertAttributeCurr->type == type )
        return gltfVertAttributeCurr;
    }
    return nullptr;
  }


} // namespace Tac
