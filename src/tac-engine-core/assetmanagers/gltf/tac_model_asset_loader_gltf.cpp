#include "tac_model_asset_loader_gltf.h" // self-inc

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
#include "tac-std-lib/mutex/tac_mutex.h"


namespace Tac
{

  static void                 GetTris( const cgltf_primitive* parsedPrim,
                                       SubMeshTriangles& tris )
  {
    const cgltf_attribute* posAttribute {
      FindAttributeOfType( parsedPrim, cgltf_attribute_type_position ) };
    if( !posAttribute )
      return;

    const char* srcVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->offset +
      posAttribute->data->offset };

    const char* endVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->buffer->size };

    SubMeshTriangle tri  {};
    int iVert {};

    for( cgltf_size ii {}; ii < parsedPrim->indices->count; ++ii )
    {
      cgltf_size i{ cgltf_accessor_read_index( parsedPrim->indices, ii ) };

      const v3* vert { ( v3* )( srcVtx + posAttribute->data->stride * i ) };

      TAC_ASSERT( ( void* )vert >= ( void* )srcVtx );
      TAC_ASSERT( ( void* )vert < ( void* )endVtx );

      tri[ iVert++ ] = *vert;
      if( iVert == 3 )
      {
        iVert = 0;
        tris.push_back( tri );
      }
    }
  }

  struct LoadedGltfData
  {
    void Load( const AssetPathStringView& path, Errors& errors )
    {
      TAC_CALL( mFileBytes = LoadAssetPath( path, errors ) );

      const cgltf_options options  {};

      TAC_GLTF_CALL( cgltf_parse, &options, mFileBytes.data(), mFileBytes.size(), &parsedData );

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

    // Some of the parsed data may point into the mFileBytes string
    String      mFileBytes {};
    cgltf_data* parsedData {};
  };
  
  static Render::BufferHandle ConvertToVertexBuffer( const Render::VertexDeclarations& decls,
                                                     const cgltf_primitive* parsedPrim,
                                                     const StringView& bufferName,
                                                     Errors& errors )
  {
    const int dstVtxStride{ decls.CalculateStride() };
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

    const Render::Binding binding
    {
      Render::Binding::VertexBuffer | // IASetVertexBuffers
      Render::Binding::ShaderResource // bindless
    };

    const Render::CreateBufferParams createBufferParams
    {
      .mByteCount     { dstVtxBytes.size() },
      .mBytes         { dstVtxBytes.data() },
      .mStride        { dstVtxStride },
      .mUsage         { Render::Usage::Static },
      .mBinding       { binding },
      .mGpuBufferMode { Render::GpuBufferMode::kByteAddress },
      .mOptionalName  { bufferName },
      .mStackFrame    { TAC_STACK_FRAME },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::BufferHandle vertexBuffer {
      renderDevice->CreateBuffer( createBufferParams, errors ) };
    return vertexBuffer;
  }

  static Render::BufferHandle ConvertInputLayoutBuffer( const Render::GPUInputLayout& il, Errors& errors )
  {
    const Render::CreateBufferParams createBufferParams
    {
      .mByteCount    { sizeof( Render::GPUInputLayout ) },
      .mBytes        { &il },
      .mStride       { sizeof( Render::GPUInputLayout ) },
      .mUsage        { Render::Usage::Static },
      .mBinding      { Render::Binding::ShaderResource },
      .mGpuBufferMode{ Render::GpuBufferMode::kByteAddress },
      .mOptionalName { "input layout" },
      .mStackFrame   { TAC_STACK_FRAME },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateBuffer( createBufferParams, errors );
  }


  static Render::TexFmt ConvertIndexFormat( cgltf_component_type gltfType )
  {
    switch( gltfType )
    {
    //case cgltf_component_type_r_8u: return Render::TexFmt::kR8_uint;
    case cgltf_component_type_r_16u: return Render::TexFmt::kR16_uint;
    case cgltf_component_type_r_32u: return Render::TexFmt::kR32_uint;
    default: TAC_ASSERT_INVALID_CASE( gltfType ); return Render::TexFmt::kUnknown;
    }
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

    const Render::TexFmt fmt{ ConvertIndexFormat( indices->component_type ) };
    TAC_RAISE_ERROR_IF_RETURN( {},
                               fmt == Render::TexFmt::kUnknown,
                               "unsupported index buffer type " +
                               ToString( ( int )indices->component_type ) );

    const int indexBufferByteCount{ ( int )indices->count * Render::GetTexFmtSize( fmt ) };
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
    TAC_CALL_RET( const Render::BufferHandle indexBuffer{
       device->CreateBuffer( createBufferParams, errors ) } );

    return indexBuffer;
  }

  struct GLTFVtxDecl : public Render::VertexDeclarations
  {
    GLTFVtxDecl( const cgltf_primitive* prim )
    {
      int mRunningStride{};
      for( cgltf_size i{}; i < prim->attributes_count; ++i )
      {
        const cgltf_attribute& attribute{ prim->attributes[i] };
        const cgltf_type gltfType{ attribute.data->type };
        const cgltf_attribute_type gltfAttribType{ attribute.type };
        const Render::Attribute tacAttribType{ GLTFToTacAttribute( gltfAttribType ) };

        Render::VertexAttributeFormat tacVAF{};
        if( gltfType == cgltf_type_vec2 )
          tacVAF = Render::VertexAttributeFormat::GetVector2();
        else if ( gltfType == cgltf_type_vec3 )
          tacVAF = Render::VertexAttributeFormat::GetVector3();
        else
        {
          TAC_ASSERT_UNIMPLEMENTED;
        }

        const Render::VertexDeclaration vtxDecl
        {
          .mAttribute         { tacAttribType },
          .mFormat            { tacVAF },
          .mAlignedByteOffset { mRunningStride },
        };
        push_back( vtxDecl );

        mRunningStride += vtxDecl.mFormat.CalculateTotalByteCount();
      }
    }
  };

  static void                 PopulateSubmeshes( Vector< SubMesh >& submeshes,
                                                 const AssetPathStringView& path,
                                                 const int specifiedMeshIndex,
                                                 dynmc Render::VertexDeclarations& vtxDecls,
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

        if( vtxDecls.empty() )
          vtxDecls = GLTFVtxDecl( parsedPrim );

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

        TAC_CALL( const Render::BufferHandle indexBuffer{
          ConvertToIndexBuffer( parsedPrim, bufferName, errors ) } );

        TAC_CALL( const Render::BufferHandle vertexBuffer{
          ConvertToVertexBuffer( vtxDecls, parsedPrim, bufferName, errors ) } );


        SubMeshTriangles tris;
        GetTris( parsedPrim, tris );

        const Render::PrimitiveTopology topology { Render::PrimitiveTopology::TriangleList };
        const cgltf_primitive_type supportedType { GetGltfFromTopology( topology ) };
        TAC_ASSERT( parsedPrim->type == supportedType );

        Render::IBindlessArray* bindlessArray{ ModelAssetManager::GetBindlessArray() };
        TAC_CALL( const Render::IBindlessArray::Binding vtxBufBinding{
          bindlessArray->Bind( vertexBuffer, errors ) } );

        const SubMesh subMesh
        {
          .mPrimitiveTopology    { topology },
          .mVertexBuffer         { vertexBuffer },
          .mIndexBuffer          { indexBuffer },
          .mVertexBufferBinding  { vtxBufBinding },
          .mTris                 { tris },
          .mIndexCount           { indexCount },
          .mVertexCount          { vertexCount },
          .mName                 { StringView( bufferName ) },
        };

        submeshes.push_back( subMesh );

      }
    }
  }

  static Mesh                 LoadMeshFromGltf( ModelAssetManager::Params params,
                                                Errors& errors )
  {
// temp temp temp
    static Mutex m;
    TAC_SCOPE_GUARD( LockGuard, m );
// temp temp temp

    const AssetPathStringView& path{ params.mPath };
    const int specifiedMeshIndex{ params.mModelIndex };
    dynmc Render::VertexDeclarations vtxDecls{ params.mOptVtxDecls };

    Vector< SubMesh > submeshes;

    TAC_CALL_RET( PopulateSubmeshes( submeshes, path, specifiedMeshIndex, vtxDecls, errors ) );

    const Render::GPUInputLayout gpuInputLayout( vtxDecls );

    TAC_CALL_RET( const Render::BufferHandle gpuInputLayoutBuffer{
      ConvertInputLayoutBuffer( gpuInputLayout, errors ) } );

    Render::IBindlessArray* bindlessArray{ ModelAssetManager::GetBindlessArray() };
    TAC_CALL_RET( const Render::IBindlessArray::Binding gpuInputLayoutBinding{
      bindlessArray->Bind( gpuInputLayoutBuffer, errors ) } );

    return Mesh
    {
      .mSubMeshes             { submeshes },
      .mVertexDecls           { vtxDecls },
      .mGPUInputLayout        { gpuInputLayout },
      .mGPUInputLayoutBuffer  { gpuInputLayoutBuffer },
      .mGPUInputLayoutBinding { gpuInputLayoutBinding },
    };
  }



  void                        GltfLoaderInit()
  {
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".gltf" );
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".glb" );
  }


} // namespace Tac
