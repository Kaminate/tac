#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/tacMemory.h"
#include "common/tacJobQueue.h"
#include "common/tacRenderer.h"
#include "common/tacUtility.h"
#include "common/tacOS.h"

#pragma warning( push )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4244 )
#define CGLTF_IMPLEMENTATION
#include "common/thirdparty/cgltf.h"
#pragma warning( pop )

struct TacLoadingMesh
{
};

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

void TacFillDataType( cgltf_component_type component_type, TacFormat* dataType )
{
  switch( component_type )
  {
  case cgltf_component_type_r_16u:
    dataType->mElementCount = 1;
    dataType->mPerElementByteCount = 2;
    dataType->mPerElementDataType = TacGraphicsType::uint;
    break;
  default:
    TacInvalidCodePath;
  }
}

void TacModelAssetManager::GetMesh( TacMesh** mesh, const TacString& path, TacErrors& errors )
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

  for( int iMesh = 0; iMesh < parsedData->meshes_count; ++iMesh )
  {
    cgltf_mesh* parsedMesh = &parsedData->meshes[ iMesh ];

    for( int iPrim = 0; iPrim < parsedMesh->primitives_count; ++iPrim )
    {
      cgltf_primitive* parsedPrim = &parsedMesh->primitives[ iPrim ];

      cgltf_accessor* indices = parsedPrim->indices;

      TacAssert( indices->type == cgltf_type_scalar );
      TacFormat format;
      TacFillDataType( indices->component_type, &format );

      TacIndexBuffer* indexBuffer;
      TacIndexBufferData indexBufferData = {};
      indexBufferData.indexCount = ( int )indices->count;
      indexBufferData.access = TacAccess::Default;
      indexBufferData.mStackFrame = TAC_STACK_FRAME;
      indexBufferData.mName = debugName;
      indexBufferData.data = indices->buffer_view->buffer->data;
      indexBufferData.dataType = format;
      TacErrors indexBufferErrors;
      mRenderer->AddIndexBuffer( &indexBuffer, indexBufferData, indexBufferErrors );


      TacVertexBuffer* vertexBuffer;
      TacVertexBufferData vertexBufferData = {};
      vertexBufferData.access = TacAccess::Default;
      vertexBufferData.mName = debugName;
      vertexBufferData.mStackFrame = TAC_STACK_FRAME;
      vertexBufferData.mNumVertexes;
      //mRenderer->AddVertexBuffer(

      // vertex format?

        TacUnimplemented;
      std::cout << "Asdf";
    }
  }

  auto newMesh = new TacMesh;

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
