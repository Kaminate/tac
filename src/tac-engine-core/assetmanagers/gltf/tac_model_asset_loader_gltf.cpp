#include "tac_model_asset_loader_gltf.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_model_load_synchronous.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager_backend.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{

  // This function converts indexes from the given T into int.
  template< typename T >
  static Vector< int >        ConvertIndexes( const cgltf_accessor* indices )
  {
    const int n{( int )indices->count};
    T* indiciesData { ( T* )( ( char* )
                                indices->buffer_view->buffer->data +
                                indices->buffer_view->offset ) };
    Vector< int > result( n );
    for( int i{}; i < n; ++i )
    {
      // Here we are casting from T to int
      result[ i ] = ( int )indiciesData[ i ];
    }
    
    return result;
  }

  static Vector< int >        ConvertIndexes( const cgltf_accessor* indices )
  {
    switch( indices->component_type )
    {
      case cgltf_component_type_r_8:   return ConvertIndexes< i8 >( indices );    break;
      case cgltf_component_type_r_8u:  return ConvertIndexes< u8 >( indices );    break;
      case cgltf_component_type_r_16:  return ConvertIndexes< i16 >( indices );   break;
      case cgltf_component_type_r_16u: return ConvertIndexes< u16 >( indices );   break;
      case cgltf_component_type_r_32u: return ConvertIndexes< u32 >( indices );   break;
      case cgltf_component_type_r_32f: return ConvertIndexes< float >( indices ); break;
      default: return {};
    }
  }

  static void                 GetTris( const cgltf_primitive* parsedPrim,
                                       SubMeshTriangles& tris )
  {
    const cgltf_attribute* posAttribute {
      FindAttributeOfType( parsedPrim, cgltf_attribute_type_position ) };
    if( !posAttribute )
      return;

    const Vector< int > indexes { ConvertIndexes( parsedPrim->indices ) };
    if( indexes.empty() )
      return;

    const char* srcVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->offset +
      posAttribute->data->offset };
    SubMeshTriangle tri  {};
    int iVert {};
    for( int i : indexes )
    {
      const v3* vert { ( v3* )( srcVtx + posAttribute->data->stride * i ) };
      tri[ iVert++ ] = *vert;
      if( iVert == 3 )
      {
        iVert = 0;
        tris.push_back( tri );
      }
    }
  }


  static int ComputeStride( const Render::VertexDeclarations& vertexDeclarations )
  {
    int dstVtxStride {};

    for( const Render::VertexDeclaration& decl : vertexDeclarations )
    {
      const int vertexEnd{
        decl.mAlignedByteOffset +
        decl.mFormat.CalculateTotalByteCount() };
      dstVtxStride = Max( dstVtxStride, vertexEnd );
    }

    TAC_ASSERT( dstVtxStride );
    return dstVtxStride;
  }


  struct LoadedGltfData
  {
    void Load( const AssetPathStringView& path, Errors& errors )
    {
      TAC_CALL( const String bytes{ LoadAssetPath( path, errors ) } );

      const cgltf_options options  {};

      TAC_GLTF_CALL( cgltf_parse, &options, bytes.data(), bytes.size(), &parsedData );

      TAC_GLTF_CALL( cgltf_validate, parsedData );

      // gltf directories must end with '/'
      const ShortFixedString basePath{ ShortFixedString::Concat( path.GetDirectory(),
                                                                  StringView( "/" ) ) };

      TAC_GLTF_CALL( cgltf_load_buffers, &options, parsedData, basePath.data() );
    }

    ~LoadedGltfData()
    {
      cgltf_free( parsedData );
    }

    cgltf_data* parsedData { nullptr };
  };
  
  static Render::BufferHandle ConvertToVertexBuffer( const Render::VertexDeclarations& decls,
                                                     const cgltf_primitive* parsedPrim,
                                                     const StringView& bufferName,
                                                     Errors& errors )
  {
    const int dstVtxStride { ComputeStride( decls ) };
    TAC_ASSERT( dstVtxStride );

    const int vertexCount { ( int )parsedPrim->attributes[ 0 ].data->count };
    Vector< char > dstVtxBytes( vertexCount * dstVtxStride, ( char )0 );

    for( int iVertexDeclaration {};
         iVertexDeclaration < decls.size();
         iVertexDeclaration++ )
    {
      const Render::VertexDeclaration& vertexDeclaration { decls[ iVertexDeclaration ] };
      const Render::VertexAttributeFormat& dstFormat { vertexDeclaration.mFormat };
      const cgltf_attribute_type gltfVertAttributeType {
        GetGltfFromAttribute( vertexDeclaration.mAttribute ) };
      const cgltf_attribute* gltfVertAttribute {
        FindAttributeOfType( parsedPrim, gltfVertAttributeType ) };
      if( !gltfVertAttribute )
        continue;

      const cgltf_accessor* gltfVertAttributeData { gltfVertAttribute->data };
      const Render::VertexAttributeFormat srcFormat { FillDataType( gltfVertAttributeData ) };
      TAC_ASSERT( vertexCount == ( int )gltfVertAttributeData->count );
      char* dstVtx { dstVtxBytes.data() };
      char* srcVtx{ ( char* )gltfVertAttributeData->buffer_view->buffer->data +
        gltfVertAttributeData->offset +
        gltfVertAttributeData->buffer_view->offset };
      const int elementCount { Min( dstFormat.mElementCount, srcFormat.mElementCount ) };
      for( int iVert{}; iVert < vertexCount; ++iVert )
      {
        char* srcElement { srcVtx };
        char* dstElement { dstVtx + vertexDeclaration.mAlignedByteOffset };
        for( int iElement {}; iElement < elementCount; ++iElement )
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

    TAC_ASSERT( dstVtxStride );
    TAC_ASSERT( dstVtxBytes.size() );
    TAC_UNUSED_PARAMETER( dstVtxStride );

    const Render::CreateBufferParams createBufferParams
    {
      .mByteCount    { dstVtxBytes.size() },
      .mBytes        { dstVtxBytes.data() },
      .mStride       { dstVtxStride },
      .mUsage        { Render::Usage::Static },
      .mBinding      { Render::Binding::VertexBuffer },
      .mOptionalName { bufferName },
      .mStackFrame   { TAC_STACK_FRAME },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::BufferHandle vertexBuffer {
      renderDevice->CreateBuffer( createBufferParams, errors ) };
    return vertexBuffer;
  }


  static Render::BufferHandle ConvertToIndexBuffer( const cgltf_primitive* parsedPrim,
                                                    const StringView& bufferName,
                                                    Errors& errors )
  {
    TAC_ASSERT( parsedPrim->indices->type == cgltf_type_scalar );

    const cgltf_accessor* indices { parsedPrim->indices };
    const void* indiciesData{ ( char* )
      indices->buffer_view->buffer->data +
      indices->buffer_view->offset };

    TAC_ASSERT( indices->type == cgltf_type_scalar );
    TAC_ASSERT( indices->component_type == cgltf_component_type_r_16u );
    const Render::TexFmt fmt{ Render::TexFmt::kR16_uint };

    const int indexBufferByteCount { ( int )indices->count * (int)sizeof( u16 ) };
    const Render::CreateBufferParams createBufferParams
    {
      .mByteCount    { indexBufferByteCount },
      .mBytes        { indiciesData },
      .mUsage        { Render::Usage::Static },
      .mBinding      { Render::Binding::IndexBuffer },
      .mGpuBufferFmt { fmt },
      .mOptionalName { bufferName },
      .mStackFrame   { TAC_STACK_FRAME },
    };
    Render::IDevice* device { Render::RenderApi::GetRenderDevice() };
    TAC_CALL_RET( {}, const Render::BufferHandle indexBuffer{
       device->CreateBuffer( createBufferParams, errors ) } );

    return indexBuffer;
  }

  static void                 PopulateSubmeshes( Vector< SubMesh >& submeshes,
                                                 const AssetPathStringView& path,
                                                 const int specifiedMeshIndex,
                                                 const Render::VertexDeclarations& vtxDecls,
                                                 Errors& errors )
  {
    LoadedGltfData loadedData;
    TAC_CALL( loadedData.Load( path, errors ));

    cgltf_data* parsedData { loadedData.parsedData };

    const int meshCount { ( int )parsedData->meshes_count };
    TAC_ASSERT_INDEX( specifiedMeshIndex, meshCount ); // like what r u trying to load bro

    for( int iMesh {}; iMesh < meshCount; ++iMesh )
    {
      cgltf_mesh* parsedMesh { &parsedData->meshes[ iMesh ] };
      if( iMesh != specifiedMeshIndex )
        continue;

      const int primitiveCount { ( int )parsedMesh->primitives_count };
      for( int iPrim{}; iPrim < primitiveCount; ++iPrim )
      {
        cgltf_primitive* parsedPrim{ &parsedMesh->primitives[ iPrim ] };
        if( !parsedPrim->attributes_count )
          continue;

        TAC_ASSERT( parsedPrim->indices->type == cgltf_type_scalar );

        ShortFixedString bufferName{ path.GetFilename() };
        if( !( meshCount == 1 && primitiveCount == 1 ) )
        {
          bufferName += ":";
          bufferName += ToString( iMesh );
          bufferName += ":";
          bufferName += ToString( iPrim );
        }

        const int vertexCount{ ( int )parsedPrim->attributes[ 0 ].data->count };
        const int indexCount{ ( int )parsedPrim->indices->count };

        TAC_CALL( const Render::BufferHandle indexBuffer {
                  ConvertToIndexBuffer( parsedPrim, bufferName, errors ) } );

        TAC_CALL( const Render::BufferHandle vertexBuffer{
          ConvertToVertexBuffer( vtxDecls, parsedPrim, bufferName, errors ) } );

        SubMeshTriangles tris;
        GetTris( parsedPrim, tris );

        const Render::PrimitiveTopology topology { Render::PrimitiveTopology::TriangleList };
        const cgltf_primitive_type supportedType { GetGltfFromTopology( topology ) };
        TAC_ASSERT( parsedPrim->type == supportedType );

        const SubMesh subMesh
        {
          .mPrimitiveTopology { topology },
          .mVertexBuffer      { vertexBuffer },
          .mIndexBuffer       { indexBuffer },
          .mTris              { tris },
          .mIndexCount        { indexCount },
          .mVertexCount       { vertexCount },
          .mName              { StringView( bufferName ) },
        };
        submeshes.push_back( subMesh );
      }
    }
  }

  static Mesh                 LoadMeshFromGltf( const AssetPathStringView& path,
                                                const int specifiedMeshIndex,
                                                const Render::VertexDeclarations& vtxDecls,
                                                Errors& errors )
  {
    Vector< SubMesh > submeshes;

    TAC_CALL_RET( {}, PopulateSubmeshes( submeshes, path, specifiedMeshIndex, vtxDecls, errors ) );

    return Mesh
    {
      .mSubMeshes { submeshes },
    };
  }



  void                        GltfLoaderInit()
  {
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".gltf" );
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".glb" );
  }


} // namespace Tac
