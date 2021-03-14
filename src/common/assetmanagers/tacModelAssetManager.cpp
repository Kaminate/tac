#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/tacMemory.h"
#include "src/common/tacJobQueue.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacUtility.h"

namespace Tac
{


#pragma warning( push )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4100 )
#define CGLTF_IMPLEMENTATION
#include "src/common/thirdparty/cgltf.h"
#pragma warning( pop )


  static std::map< StringID, Mesh* > mMeshes;

  static cgltf_attribute_type GetGltfFromAttribute( Attribute attributeType )
  {
    switch( attributeType )
    {
      case Attribute::Position: return cgltf_attribute_type_position;
      case Attribute::Normal: return cgltf_attribute_type_normal;
      case Attribute::Texcoord: return cgltf_attribute_type_texcoord;
      case Attribute::Color: return cgltf_attribute_type_color;
      case Attribute::BoneIndex: return cgltf_attribute_type_joints;
      case Attribute::BoneWeight: return cgltf_attribute_type_weights;
      default: TAC_ASSERT_INVALID_CASE( attributeType ); return cgltf_attribute_type_invalid;
    }
  }

  // replace with above?
  //static Attribute GetAttributeFromGltf( cgltf_attribute_type attributeType )
  //{
  //  switch( attributeType )
  //  {
  //    case cgltf_attribute_type_position: return Attribute::Position;
  //    case cgltf_attribute_type_normal: return Attribute::Normal;
  //    case cgltf_attribute_type_texcoord: return Attribute::Texcoord;
  //    case cgltf_attribute_type_color: return Attribute::Color;
  //    case cgltf_attribute_type_joints: return Attribute::BoneIndex;
  //    case cgltf_attribute_type_weights: return Attribute::BoneWeight;
  //    default: TAC_ASSERT_INVALID_CASE( attributeType ); return Attribute::Count;
  //  }
  //}

  static const char*          GetcgltfErrorAsString( cgltf_result parseResult )
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

  static int                  FillDataTypePerElementByteCount( cgltf_accessor* accessor )
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return 2;
      case cgltf_component_type_r_32f: return 4;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ); return 0;
    }
  }

  static Tac::GraphicsType    FillDataTypePerElementDataType( cgltf_accessor* accessor )
  {
    switch( accessor->component_type )
    {
      case cgltf_component_type_r_16u: return GraphicsType::uint;
      case cgltf_component_type_r_32f: return GraphicsType::real;
      default: TAC_ASSERT_INVALID_CASE( accessor->component_type ) return ( GraphicsType )0;
    }

  }

  static int                  FillDataTypeElementCount( cgltf_accessor* accessor )
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

  static Format               FillDataType( cgltf_accessor* accessor )
  {
    Format result = {};
    result.mElementCount = FillDataTypeElementCount( accessor );
    result.mPerElementByteCount = FillDataTypePerElementByteCount( accessor );
    result.mPerElementDataType = FillDataTypePerElementDataType( accessor );
    return result;
  }

  static cgltf_attribute*     FindAttributeOfType( cgltf_primitive* parsedPrim, cgltf_attribute_type type )
  {
    for( int iAttrib = 0; iAttrib < ( int )parsedPrim->attributes_count; ++iAttrib )
    {
      cgltf_attribute* gltfVertAttributeCurr = &parsedPrim->attributes[ iAttrib ];
      if( gltfVertAttributeCurr->type == type )
        return gltfVertAttributeCurr;
    }
    return nullptr;
  }

  template< typename T >
  static Vector< int >        ConvertIndexes( cgltf_accessor* indices )
  {
    auto indiciesData = ( T* )( ( char* )indices->buffer_view->buffer->data + indices->buffer_view->offset );
    Vector< int > result( ( int )indices->count );
    for( int i = 0; i < ( int )indices->count; ++i )
      result[ i ] = ( int )indiciesData[ i ];
    return result;
  }

  static void                 GetTris( cgltf_primitive* parsedPrim, SubMeshTriangles& tris )
  {
    cgltf_attribute* posAttribute = FindAttributeOfType( parsedPrim, cgltf_attribute_type_position );
    if( !posAttribute )
      return;

    Vector< int > indexes;
    switch( parsedPrim->indices->component_type )
    {
      case cgltf_component_type_r_8: indexes = ConvertIndexes<int8_t>( parsedPrim->indices ); break;
      case cgltf_component_type_r_8u: indexes = ConvertIndexes<uint8_t>( parsedPrim->indices ); break;
      case cgltf_component_type_r_16: indexes = ConvertIndexes<int16_t>( parsedPrim->indices ); break;
      case cgltf_component_type_r_16u: indexes = ConvertIndexes<uint16_t>( parsedPrim->indices ); break;
      case cgltf_component_type_r_32u: indexes = ConvertIndexes<uint32_t>( parsedPrim->indices ); break;
      case cgltf_component_type_r_32f: indexes = ConvertIndexes<float>( parsedPrim->indices ); break;
      default: break; // do nothing
    }
    if( indexes.empty() )
      return;

    auto srcVtx = ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->offset +
      posAttribute->data->offset;
    SubMeshTriangle tri = {};
    int iVert = 0;
    for( int i : indexes )
    {
      auto vert = ( v3* )( srcVtx + posAttribute->data->stride * i );
      tri[ iVert++ ] = *vert;
      if( iVert == 3 )
      {
        iVert = 0;
        tris.push_back( tri );
      }
    }
  }

  static bool RaycastTriangle( const v3& p0,
                               const v3& p1,
                               const v3& p2,
                               const v3& rayPos,
                               const v3& normalizedRayDir,
                               float & dist )
  {
    v3 edge2 = p2 - p0;
    v3 edge1 = p1 - p0;
    v3 b = rayPos - p0;
    v3 p = Cross( normalizedRayDir, edge2 );
    v3 q = Cross( b, edge1 );
    float pdotv1 = Dot( p, edge1 );
    float t = Dot( q, edge2 ) / pdotv1;
    float u = Dot( p, b ) / pdotv1;
    float v = Dot( q, normalizedRayDir ) / pdotv1;
    if( t > 0 && u >= 0 && v >= 0 && u + v <= 1 )
    {
      dist = t;
      return true;
    }
    return false;
  }

  void ModelAssetManagerUninit()
  {
    for( auto pair : mMeshes )
    {
      Mesh* mesh = pair.second;
      for( SubMesh& submesh : mesh->mSubMeshes )
      {
        Render::DestroyIndexBuffer( submesh.mIndexBuffer, TAC_STACK_FRAME );
        Render::DestroyVertexBuffer( submesh.mVertexBuffer, TAC_STACK_FRAME );
      }
    }
  }

  // todo: multithreading
  void ModelAssetManagerGetMesh( Mesh** mesh,
                                 StringView path,
                                 //Render::VertexFormatHandle vertexFormat,
                                 const VertexDeclarations& vertexDeclarations,
                                 //VertexDeclaration* vertexDeclarations,
                                 //int vertexDeclarationCount,
                                 Errors& errors )
  {
    auto it = mMeshes.find( path );
    if( it != mMeshes.end() )
    {
      *mesh = ( *it ).second;
      return;
    }

    auto bytes = TemporaryMemoryFromFile( path, errors );
    TAC_HANDLE_ERROR( errors );

    cgltf_options options = {};

    cgltf_data* parsedData;
    cgltf_result parseResult = cgltf_parse( &options, bytes.data(), bytes.size(), &parsedData );
    if( parseResult != cgltf_result_success )
    {
      const String errorMsg = String( "cgltf_parse: " ) + GetcgltfErrorAsString( parseResult );
      TAC_RAISE_ERROR( errorMsg, errors );
    }
    TAC_ON_DESTRUCT( cgltf_free( parsedData ) );

    parseResult = cgltf_validate( parsedData );
    if( parseResult != cgltf_result_success )
    {
      const String errorMsg = String( "cgltf_validate: " ) + GetcgltfErrorAsString( parseResult );
      TAC_RAISE_ERROR( errorMsg, errors );
    }


    SplitFilepath splitFilepath( path );
    String debugName = StripExt( splitFilepath.mFilename );

    parseResult = cgltf_load_buffers( &options, parsedData, path.c_str() );//splitFilepath.mDirectory.c_str() );
    if( parseResult != cgltf_result_success )
    {
      const String errorMsg = String( "cgltf_validate: " ) + GetcgltfErrorAsString( parseResult );
      TAC_RAISE_ERROR( errorMsg, errors );
    }

    Vector< SubMesh > submeshes;

    for( int iMesh = 0; iMesh < ( int )parsedData->meshes_count; ++iMesh )
    {
      cgltf_mesh* parsedMesh = &parsedData->meshes[ iMesh ];

      for( int iPrim = 0; iPrim < ( int )parsedMesh->primitives_count; ++iPrim )
      {
        cgltf_primitive* parsedPrim = &parsedMesh->primitives[ iPrim ];
        if( !parsedPrim->attributes_count )
          continue;

        cgltf_accessor* indices = parsedPrim->indices;
        void* indiciesData = ( char* )indices->buffer_view->buffer->data + indices->buffer_view->offset;
        TAC_ASSERT( indices->type == cgltf_type_scalar );
        const Format indexFormat = FillDataType( indices );
        const int indexByteCount = ( int )indices->count * ( int )sizeof( indexFormat.CalculateTotalByteCount() );
        Render::IndexBufferHandle indexBuffer = Render::CreateIndexBuffer( indexByteCount,
                                                                           indiciesData,
                                                                           Access::Default,
                                                                           indexFormat,
                                                                           TAC_STACK_FRAME );

        int vertexCount = ( int )parsedPrim->attributes[ 0 ].data->count;
        int dstVtxStride = 0;

        for( int iVertexDeclaration = 0; iVertexDeclaration < vertexDeclarations.size(); ++iVertexDeclaration )
        {
          const VertexDeclaration& vertexDeclaration = vertexDeclarations[ iVertexDeclaration ];
          int vertexEnd =
            vertexDeclaration.mAlignedByteOffset +
            vertexDeclaration.mTextureFormat.CalculateTotalByteCount();
          dstVtxStride = Max( dstVtxStride, vertexEnd );
        }
        Vector< char > dstVtxBytes( vertexCount * dstVtxStride, ( char )0 );

        for( int iVertexDeclaration = 0; iVertexDeclaration < vertexDeclarations.size(); ++iVertexDeclaration )
        {
          const VertexDeclaration& vertexDeclaration = vertexDeclarations[ iVertexDeclaration ];
          const Format& dstFormat = vertexDeclaration.mTextureFormat;
          cgltf_attribute_type gltfVertAttributeType = GetGltfFromAttribute( vertexDeclaration.mAttribute );
          cgltf_attribute* gltfVertAttribute = FindAttributeOfType( parsedPrim, gltfVertAttributeType );
          if( !gltfVertAttribute )
            continue;
          cgltf_accessor* gltfVertAttributeData = gltfVertAttribute->data;
          Format srcFormat = FillDataType( gltfVertAttributeData );
          TAC_ASSERT( vertexCount == ( int )gltfVertAttributeData->count );
          char* dstVtx = dstVtxBytes.data();
          char* srcVtx = ( char* )gltfVertAttributeData->buffer_view->buffer->data +
            gltfVertAttributeData->offset +
            gltfVertAttributeData->buffer_view->offset;
          int elementCount = Min( dstFormat.mElementCount, srcFormat.mElementCount );
          for( int iVert = 0; iVert < vertexCount; ++iVert )
          {
            char* srcElement = srcVtx + vertexDeclaration.mAlignedByteOffset;
            char* dstElement = dstVtx + 0;
            for( int iElement = 0; iElement < elementCount; ++iElement )
            {
              if( srcFormat.mPerElementDataType == dstFormat.mPerElementDataType &&
                  srcFormat.mPerElementByteCount == dstFormat.mPerElementByteCount )
              {
                MemCpy( dstElement, srcElement, srcFormat.mPerElementByteCount );
              }
              else
              {
                TAC_ASSERT_UNIMPLEMENTED;
              }
              // copy
              dstElement += dstFormat.mPerElementByteCount;
              srcElement += srcFormat.mPerElementByteCount;
            }
            srcVtx += gltfVertAttributeData->stride;
            dstVtx += dstVtxStride;
          }
        }


        const Render::VertexBufferHandle vertexBuffer = Render::CreateVertexBuffer( dstVtxBytes.size(),
                                                                                    dstVtxBytes.data(),
                                                                                    dstVtxStride,
                                                                                    Access::Default,
                                                                                    TAC_STACK_FRAME );

        SubMeshTriangles tris;
        GetTris( parsedPrim, tris );

        const String name = parsedMesh->name +
          ( parsedMesh->primitives_count > 1 ? "prim" + ToString( iPrim ) : "" );

        SubMesh subMesh;
        subMesh.mIndexBuffer = indexBuffer;
        subMesh.mVertexBuffer = vertexBuffer;
        subMesh.mTris = tris;
        subMesh.mIndexCount = ( int )indices->count;
        subMesh.mName = name;
        submeshes.push_back( subMesh );
      }
    }

    m4 transform = m4::Identity();
    m4 transformInv = m4::Identity();
    cgltf_node* node = parsedData->scene->nodes[ 0 ];
    if( node->has_translation )
    {
      v3 pos = { node->translation[ 0 ], node->translation[ 1 ], node->translation[ 2 ] };
      transform = m4::Translate( pos );
      transformInv = m4::Translate( -pos );
    }

    auto newMesh = TAC_NEW Mesh;
    newMesh->mSubMeshes = submeshes;
    //newMesh->mVertexFormat = vertexFormat;
    newMesh->mTransform = transform;
    newMesh->mTransformInv = transformInv;
    *mesh = newMesh;

    mMeshes[ path ] = newMesh;
  }


  void SubMesh::SubMeshModelSpaceRaycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist )
  {
    bool submeshHit = false;
    float submeshDist = 0;
    int triCount = ( int )mTris.size();
    for( int iTri = 0; iTri < triCount; ++iTri )
    {
      const SubMeshTriangle& tri = mTris[ iTri ];
      float triDist;
      const bool triHit = RaycastTriangle( tri[ 0 ],
                                           tri[ 1 ],
                                           tri[ 2 ],
                                           inRayPos,
                                           inRayDir,
                                           triDist );
      if( !triHit )
        continue;
      if( submeshHit && triDist > submeshDist )
        continue;
      submeshDist = triDist;
      submeshHit = true;
    }
    *outHit = submeshHit;
    *outDist = submeshDist;
  }

  void Mesh::MeshModelSpaceRaycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist )
  {
    v4 inRayPos4 = { inRayPos, 1 };
    inRayPos4 = mTransformInv * inRayPos4;
    inRayPos = inRayPos4.xyz();

    bool meshHit = false;
    float meshDist = 0;
    for( SubMesh& subMesh : mSubMeshes )
    {
      bool subMeshHit = false;
      float submeshDist = 0;
      subMesh.SubMeshModelSpaceRaycast( inRayPos, inRayDir, &subMeshHit, &submeshDist );
      if( !subMeshHit )
        continue;
      if( meshHit && submeshDist > meshDist )
        continue;
      meshDist = submeshDist;
      meshHit = true;
    }
    *outHit = meshHit;
    *outDist = meshDist;
  }

  //v3 GetNormal( const SubMeshTriangle& tri )
  //{
  //  v3 edge0 = tri[ 1 ] - tri[ 0 ];
  //  v3 edge1 = tri[ 2 ] - tri[ 0 ];
  //  // check for div 0?
  //  return Normalize( Cross( edge0, edge1 ) );
  //}
}
