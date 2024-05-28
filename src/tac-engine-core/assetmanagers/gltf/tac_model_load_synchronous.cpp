#include "tac_model_load_synchronous.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/tac_ints.h"


namespace Tac
{



  template< typename T >
  static Vector< int >        ConvertIndexes( cgltf_accessor* indices )
  {
    auto indiciesData { ( T* )( ( char* )indices->buffer_view->buffer->data + indices->buffer_view->offset ) };
    Vector< int > result( ( int )indices->count );
    for( int i{}; i < ( int )indices->count; ++i )
      result[ i ] = ( int )indiciesData[ i ];
    return result;
  }

  static void                 GetTris( const cgltf_primitive* parsedPrim, SubMeshTriangles& tris )
  {
    const cgltf_attribute* posAttribute{
      FindAttributeOfType( parsedPrim, cgltf_attribute_type_position ) };
    if( !posAttribute )
      return;

    Vector< int > indexes;
    switch( parsedPrim->indices->component_type )
    {
      case cgltf_component_type_r_8:   indexes = ConvertIndexes< i8 >( parsedPrim->indices ); break;
      case cgltf_component_type_r_8u:  indexes = ConvertIndexes< u8 >( parsedPrim->indices ); break;
      case cgltf_component_type_r_16:  indexes = ConvertIndexes< i16 >( parsedPrim->indices ); break;
      case cgltf_component_type_r_16u: indexes = ConvertIndexes< u16 >( parsedPrim->indices ); break;
      case cgltf_component_type_r_32u: indexes = ConvertIndexes< u32 >( parsedPrim->indices ); break;
      case cgltf_component_type_r_32f: indexes = ConvertIndexes< float >( parsedPrim->indices ); break;
      default: break; // do nothing
    }
    if( indexes.empty() )
      return;

    auto srcVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->offset +
      posAttribute->data->offset };
    SubMeshTriangle tri  {};
    int iVert {};
    for( int i : indexes )
    {
      auto vert { ( v3* )( srcVtx + posAttribute->data->stride * i ) };
      tri[ iVert++ ] = *vert;
      if( iVert == 3 )
      {
        iVert = 0;
        tris.push_back( tri );
      }
    }
  }

}
