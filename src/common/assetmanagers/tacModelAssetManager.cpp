#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/tacMemory.h"
#include "common/tacJobQueue.h"
#include "common/graphics/tacRenderer.h"
#include "common/math/tacMath.h"
#include "common/tacUtility.h"

#pragma warning( push )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#define CGLTF_IMPLEMENTATION
#include "common/thirdparty/cgltf.h"
#pragma warning( pop )

static cgltf_attribute_type GetGltfFromAttribute( TacAttribute attributeType )
{
  switch( attributeType )
  {
  case TacAttribute::Position: return cgltf_attribute_type_position;
  case TacAttribute::Normal: return cgltf_attribute_type_normal;
  case TacAttribute::Texcoord: return cgltf_attribute_type_texcoord;
  case TacAttribute::Color: return cgltf_attribute_type_color;
  case TacAttribute::BoneIndex: return cgltf_attribute_type_joints;
  case TacAttribute::BoneWeight: return cgltf_attribute_type_weights;
      TacInvalidDefaultCase( attributeType );
  }
  return cgltf_attribute_type_invalid;
}

// replace with above?
static TacAttribute GetAttributeFromGltf( cgltf_attribute_type attributeType )
{
  switch( attributeType )
  {
  case cgltf_attribute_type_position: return TacAttribute::Position;
  case cgltf_attribute_type_normal: return TacAttribute::Normal;
  case cgltf_attribute_type_texcoord: return TacAttribute::Texcoord;
  case cgltf_attribute_type_color: return TacAttribute::Color;
  case cgltf_attribute_type_joints: return TacAttribute::BoneIndex;
  case cgltf_attribute_type_weights: return TacAttribute::BoneWeight;
      TacInvalidDefaultCase( attributeType );
  }
  return TacAttribute::Count;
}

static const char* GetcgltfErrorAsString( cgltf_result parseResult )
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

void TacFillDataType( cgltf_accessor* accessor, TacFormat* dataType )
{
  switch( accessor->component_type )
  {
  case cgltf_component_type_r_16u:
    dataType->mPerElementByteCount = 2;
    dataType->mPerElementDataType = TacGraphicsType::uint;
    break;
  case cgltf_component_type_r_32f:
    dataType->mPerElementByteCount = 4;
    dataType->mPerElementDataType = TacGraphicsType::real;
    break;
    TacInvalidDefaultCase( accessor->component_type );
  }
  switch( accessor->type )
  {
  case cgltf_type_scalar: dataType->mElementCount = 1; break;
  case cgltf_type_vec2: dataType->mElementCount = 2; break;
  case cgltf_type_vec3: dataType->mElementCount = 3; break;
  case cgltf_type_vec4: dataType->mElementCount = 4; break;
    TacInvalidDefaultCase( accessor->type );
  }
}

static cgltf_attribute*  FindAttributeOfType(cgltf_primitive* parsedPrim, cgltf_attribute_type  type )
{
        for( int iAttrib = 0; iAttrib < ( int )parsedPrim->attributes_count; ++iAttrib )
        {
          cgltf_attribute* gltfVertAttributeCurr = &parsedPrim->attributes[ iAttrib ];
          if( gltfVertAttributeCurr->type == type )
            return gltfVertAttributeCurr;
        }
        return nullptr;
}

void TacModelAssetManager::GetMesh( TacMesh** mesh, const TacString& path, TacVertexFormat* vertexFormat, TacErrors& errors )
{
  auto bytes = TacTemporaryMemory( path, errors );
  TAC_HANDLE_ERROR( errors );

  // zero initialize for default options, can hold allocators
  cgltf_options options = {};

  // mirrors glTF 2.0 format
  cgltf_data* parsedData;
  cgltf_result parseResult = cgltf_parse( &options, bytes.data(), bytes.size(), &parsedData );
  if( parseResult != cgltf_result_success )
  {
    errors = TacString( "cgltf_parse: " ) + GetcgltfErrorAsString( parseResult );
    return;
  }
  OnDestruct( cgltf_free( parsedData ) );

  parseResult = cgltf_validate( parsedData );
  if( parseResult != cgltf_result_success )
  {
    errors = TacString( "cgltf_validate: " ) + GetcgltfErrorAsString( parseResult );
    return;
  }


  TacSplitFilepath splitFilepath( path );
  TacString debugName = TacStripExt( splitFilepath.mFilename );

  parseResult = cgltf_load_buffers( &options, parsedData, path.c_str() );//splitFilepath.mDirectory.c_str() );
  if( parseResult != cgltf_result_success )
  {
    errors = TacString( "cgltf_validate: " ) + GetcgltfErrorAsString( parseResult );
    return;
  }

  TacVector< TacSubMesh > submeshes;

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
      TacAssert( indices->type == cgltf_type_scalar );
      TacFormat indexFormat;
      TacFillDataType( indices, &indexFormat );
      TacIndexBuffer* indexBuffer;
      TacIndexBufferData indexBufferData = {};
      indexBufferData.indexCount = ( int )indices->count;
      indexBufferData.access = TacAccess::Default;
      indexBufferData.mStackFrame = TAC_STACK_FRAME;
      indexBufferData.mName = debugName;
      indexBufferData.data = indiciesData;
      indexBufferData.dataType = indexFormat;
      TacErrors indexBufferErrors;
      mRenderer->AddIndexBuffer( &indexBuffer, indexBufferData, indexBufferErrors );

      int vertexCount = ( int )parsedPrim->attributes[ 0 ].data->count;
      int dstVtxStride = 0;
      for( const TacVertexDeclaration& vertexDeclaration : vertexFormat->vertexFormatDatas )
      {
        int vertexEnd =
          vertexDeclaration.mAlignedByteOffset +
          vertexDeclaration.mTextureFormat.CalculateTotalByteCount();
        dstVtxStride = TacMax( dstVtxStride, vertexEnd );
      }
      TacVector< char > dstVtxs( vertexCount * dstVtxStride, ( char )0 );

      for( const TacVertexDeclaration& vertexDeclaration : vertexFormat->vertexFormatDatas )
      {
        const TacFormat& dstFormat = vertexDeclaration.mTextureFormat;
        cgltf_attribute_type gltfVertAttributeType = GetGltfFromAttribute(vertexDeclaration.mAttribute );
        cgltf_attribute* gltfVertAttribute = FindAttributeOfType( parsedPrim, gltfVertAttributeType);
        if( !gltfVertAttribute )
          continue;
        cgltf_accessor* gltfVertAttributeData = gltfVertAttribute->data;
        TacFormat srcFormat;
        TacFillDataType( gltfVertAttributeData, &srcFormat );
        TacAssert( vertexCount == ( int )gltfVertAttributeData->count );
        char* dstVtx = dstVtxs.data();
        char* srcVtx = ( char* )gltfVertAttributeData->buffer_view->buffer->data + gltfVertAttributeData->offset;
        int elementCount = TacMin( dstFormat.mElementCount, srcFormat.mElementCount );
        for( int iVert = 0; iVert < vertexCount; ++iVert )
        { 
          char* srcElement = srcVtx + vertexDeclaration.mAlignedByteOffset;
          char* dstElement = dstVtx + 0;
          for( int iElement = 0; iElement < elementCount; ++iElement )
          {
            if( srcFormat.mPerElementDataType == dstFormat.mPerElementDataType &&
              srcFormat.mPerElementByteCount == dstFormat.mPerElementByteCount )
            {
              TacMemCpy( dstElement, srcElement, srcFormat.mPerElementByteCount );
            }
            else
            {
              // todo
              TacInvalidCodePath;
            }
            // copy
            dstElement += dstFormat.mPerElementByteCount;
            srcElement += srcFormat.mPerElementByteCount;
          }
          srcVtx += gltfVertAttributeData->stride;
          dstVtx += dstVtxStride;
        }
      }

      TacVector< TacArray< v3, 3 >> tris;
      cgltf_attribute* posAttribute = FindAttributeOfType( parsedPrim, cgltf_attribute_type_position );
      if( posAttribute )
      {
          
      }

      TacVertexBuffer* vertexBuffer;
      TacVertexBufferData vertexBufferData = {};
      vertexBufferData.access = TacAccess::Default;
      vertexBufferData.mName = debugName;
      vertexBufferData.mStackFrame = TAC_STACK_FRAME;
      vertexBufferData.mNumVertexes = vertexCount;
      vertexBufferData.optionalData = dstVtxs.data();
      vertexBufferData.mStrideBytesBetweenVertexes = dstVtxStride;
      TacErrors vertexBufferErrors;
      mRenderer->AddVertexBuffer( &vertexBuffer, vertexBufferData, vertexBufferErrors );

      TacSubMesh subMesh;
      subMesh.mIndexBuffer = indexBuffer;
      subMesh.mVertexBuffer = vertexBuffer;
      subMesh.mTris = tris;
      submeshes.push_back( subMesh );
    }
  }

  auto newMesh = new TacMesh;
  newMesh->mSubMeshes = submeshes;
  newMesh->mVertexFormat = vertexFormat;
  *mesh = newMesh;

  //* `cgltf_result cgltf_load_buffers( const cgltf_options*, cgltf_data*,
  //*const char* )` can be optionally called to open and read buffer
  //* files using the `FILE*` APIs.
  //*
  //* `cgltf_result cgltf_parse_file( const cgltf_options* options, const
  //* char* path, cgltf_data** out_data )` can be used to open the given
  //* file using `FILE*` APIs and parse the data using `cgltf_parse( )`.
  //*
  //* `cgltf_node_transform_local` converts the translation / rotation / scale properties of a node
  //* into a mat4.
  //*
  //* `cgltf_node_transform_world` calls `cgltf_node_transform_local` on every ancestor in order
  //* to compute the root - to - node transformation.
  //*
  //* `cgltf_accessor_read_float` reads a certain element from an accessor and converts it to
  //* floating point, assuming that `cgltf_load_buffers` has already been called.The passed - in element
  //* size is the number of floats in the output buffer, which should be in the range[ 1, 16 ].Returns
  //* false if the passed - in element_size is too small, or if the accessor is sparse.
  //*
  //* `cgltf_accessor_read_index` is similar to its floating - point counterpart, but it returns size_t
  //* and only works with single - component data types.

}
