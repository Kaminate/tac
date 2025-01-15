#include "tac_model_asset_loader_gltf.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager_backend.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/asset/tac_asset.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/mutex/tac_mutex.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/tac_ints.h"


namespace Tac
{


  static MeshRaycast GetMeshRaycast( const cgltf_primitive* parsedPrim )
  {
    const cgltf_attribute* posAttribute {
      FindAttributeOfType( parsedPrim, cgltf_attribute_type_position ) };
    if( !posAttribute )
      return {};

    MeshRaycast tris;
    const char* srcVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->offset +
      posAttribute->data->offset };

    const char* endVtx{ ( char* )
      posAttribute->data->buffer_view->buffer->data +
      posAttribute->data->buffer_view->buffer->size };

    MeshRaycast::SubMeshTriangle tri{};
    int iVert{};

    for( cgltf_size ii{}; ii < parsedPrim->indices->count; ++ii )
    {
      cgltf_size i{ cgltf_accessor_read_index( parsedPrim->indices, ii ) };

      const v3* vert { ( v3* )( srcVtx + posAttribute->data->stride * i ) };

      TAC_ASSERT( ( void* )vert >= ( void* )srcVtx );
      TAC_ASSERT( ( void* )vert < ( void* )endVtx );

      tri[ iVert++ ] = *vert;
      if( iVert == 3 )
      {
        iVert = 0;
        tris.mTris.push_back( tri );
      }
    }
    return tris;
  }

  struct LoadedGltfData
  {
    void Load( const AssetPathStringView& path, Errors& errors )
    {
      TAC_CALL( mFileBytes = LoadAssetPath( path, errors ) );

      const cgltf_options options{};

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

      const cgltf_accessor* gltfVertAttributeData{ gltfVertAttribute->data };
      const Render::VertexAttributeFormat srcFormat{ FillDataType( gltfVertAttributeData ) };
      TAC_ASSERT( vertexCount == ( int )gltfVertAttributeData->count );
      char* dstVtx{ dstVtxBytes.data() };
      char* srcVtx{ ( char* )gltfVertAttributeData->buffer_view->buffer->data +
                    gltfVertAttributeData->offset +
                    gltfVertAttributeData->buffer_view->offset };
      const int elementCount{ Min( dstFormat.mElementCount, srcFormat.mElementCount ) };
      for( int iVert{}; iVert < vertexCount; ++iVert )
      {
        char* srcElement{ srcVtx };
        char* dstElement{ dstVtx + vertexDeclaration.mAlignedByteOffset };
        for( int iElement{}; iElement < elementCount; ++iElement )
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
    const Render::BufferHandle vertexBuffer{
      renderDevice->CreateBuffer( createBufferParams, errors ) };
    return vertexBuffer;
  }

  static Render::BufferHandle ConvertInputLayoutBuffer( const Render::GPUInputLayout& il,
                                                        Errors& errors )
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
        const Render::VertexAttributeFormat tacVAF{
          GLTFTypeToTacVertexAttributeFormat( gltfType ) };
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

  static void TryGenerateCPUMeshNormals( JPPTCPUMeshData& meshData )
  {
    if( !meshData.mNormals.empty() )
      return;

    if( meshData.mIndexes.empty() )
      return;

    const int nIndexes{ meshData.mIndexes.size() };
    TAC_ASSERT( nIndexes % 3 == 0 );
    meshData.mNormals.resize( nIndexes );
    struct TriIndexes
    {
      JPPTCPUMeshData::IndexType i0;
      JPPTCPUMeshData::IndexType i1;
      JPPTCPUMeshData::IndexType i2;
    };

    for( TriIndexes* tri{ ( TriIndexes* )meshData.mIndexes.begin() };
         tri < ( TriIndexes* )meshData.mIndexes.end();
         tri++ )
    {
      const v3 v0{ meshData.mPositions[ tri->i0 ] };
      const v3 v1{ meshData.mPositions[ tri->i1 ] };
      const v3 v2{ meshData.mPositions[ tri->i2 ] };
      const v3 normal{ Normalize( Cross( v1 - v0, v2 - v0 ) ) };
      meshData.mNormals[ tri->i0 ] = normal;
      meshData.mNormals[ tri->i1 ] = normal;
      meshData.mNormals[ tri->i2 ] = normal;
    }
  }

  static void TryGenerateCPUMeshTangents( JPPTCPUMeshData& meshData )
  {
    if( meshData.mPositions.empty() || meshData.mTexCoords.empty() || meshData.mNormals.empty() )
      return;

    Vector< v3 >& mPositions{ meshData.mPositions };
    Vector< v2 >& mTexCoords{ meshData.mTexCoords };
    Vector< v3 >& mNormals{ meshData.mNormals };
    Vector< v3i > mTriangles;
    for( int i{}; i < meshData.mIndexes.size() / 3; ++i )
    {
      const v3i tri
      {
        meshData.mIndexes[ ( i * 3 ) + 0 ],
        meshData.mIndexes[ ( i * 3 ) + 1 ],
        meshData.mIndexes[ ( i * 3 ) + 2 ],
      };
      mTriangles.push_back( tri );
    }

    //static_assert( is_same<int, JPPTCPUMeshData::IndexType>::value );

    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping

    const int nVerts{ mPositions.size() };
    const int nTexCoords{ mTexCoords.size() };
    const int nTris{ mTriangles.size() };

    Vector< v3 > tan0( nVerts, {} );
    Vector< v3 > tan1( nVerts, {} );
    meshData.mTangents.resize( nVerts );
    meshData.mBitangents.resize( nVerts );
    if( nTexCoords != nVerts )
      return;

    for( int i{}; i < nTris; ++i )
    {
      //
      //    B
      //    ^
      //    |
      //    +------------------+
      //    |                  |
      // p2 ._____             |
      //    |\    \_____       |
      //    | \         \_____ |
      //    |  \              \. p1
      //    |   \            _/|
      //    |    \         _/  |
      //    |     \      _/    |
      //    |      \   _/      |
      //    |       \ /        |
      //    +--------.---------+--> T
      //            p0
      //
      // du1 = p1.u - p0.u
      // du2 = p2.u - p0.u
      // dv1 = p1.v - p0.v
      // dv2 = p2.v - p0.v
      //
      // E1 = p1.xyz - p0.xyz
      // E2 = p2.xyz - p0.xyz
      //
      // E1 = du1 * T + dv1 * B
      // E2 = du2 * T + dv2 * B
      //
      // [ E1x E1y E1z ] = [ du1 dv1 ] [ Tx Ty Tz ]
      // [ E2x E2y E2z ]   [ du2 dv2 ] [ Bx By Bz ]
      //
      //  [ Tx Ty Tz ] =             1             [ dv2 -dv1 ] [ E1x E1y E1x ]
      //  [ Bx By Bz ]   ------------------------  [ -du2 du1 ] [ E2x E2y E2z ]
      //                 ( du1 * dv2 - du2 * dv1 )

      const v3i tri{ mTriangles[ i ] };
      const int i0{ tri[ 0 ] };
      int const i1{ tri[ 1 ] };
      const int i2{ tri[ 2 ] };

      const v3 p0{ mPositions[ i0 ] };
      const v3 p1{ mPositions[ i1 ] };
      const v3 p2{ mPositions[ i2 ] };

      const v2 uv0{ mTexCoords[ i0 ] };
      const v2 uv1{ mTexCoords[ i1 ] };
      const v2 uv2{ mTexCoords[ i2 ] };

      const v3 E1{ p1 - p0 };
      const v3 E2{ p2 - p0 };

      const float du1 { ( uv1 - uv0 )[ 0 ] };
      const float dv1 { ( uv1 - uv0 )[ 1 ] };
      const float du2 { ( uv2 - uv0 )[ 0 ] };
      const float dv2 { ( uv2 - uv0 )[ 1 ] };


      const float r{ 1 / ( du1 * dv2 - du2 * dv1 ) };

      const v3 T{ r * v3( Dot( v2{ dv2, -dv1 }, v2{ E1.x, E2.x } ),
                          Dot( v2{ dv2, -dv1 }, v2{ E1.y, E2.y } ),
                          Dot( v2{ dv2, -dv1 }, v2{ E1.z, E2.z } ) ) };
      const v3 B{ r * v3( Dot( v2{ -du2, du1 }, v2{ E1.x, E2.x } ),
                          Dot( v2{ -du2, du1 }, v2{ E1.y, E2.y } ),
                          Dot( v2{ -du2, du1 }, v2{ E1.z, E2.z } ) ) };

      // += instead of = because it will be normalized later to compute an average
      tan0[ i0 ] += T;
      tan0[ i1 ] += T;
      tan0[ i2 ] += T;

      tan1[ i0 ] += B;
      tan1[ i1 ] += B;
      tan1[ i2 ] += B;
    }

    for( int i{}; i < nVerts; ++i )
    {
      const v3 n{ mNormals[ i ] };
      v3 t{ tan0[ i ] };
      v3 b{ tan1[ i ] };

      // gram shmidt
      t = Normalize( t - n * Dot( n, t ) );
      b = Normalize( b - n * Dot( n, b ) );
      b = Normalize( b - t * Dot( t, b ) );

      meshData.mTangents[ i ] = t;
      meshData.mBitangents[ i ] = b;
    }
  }

  template< typename T >
  static void AppendCPUMeshAttribute( Vector< T >& jpptTs,
                                      const cgltf_primitive* parsedPrim,
                                      Render::Attribute attrib )
  {
    const cgltf_attribute_type gltfAttributeType{ GetGltfFromAttribute( attrib ) };
    const cgltf_attribute* gltfAttribute{ FindAttributeOfType( parsedPrim, gltfAttributeType ) };
    if( !gltfAttribute )
      return;

    const int nOldJPPTTs{ jpptTs.size() };
    jpptTs.resize( nOldJPPTTs + (int)gltfAttribute->data->count );
    T* pJpptT{ jpptTs.data() + nOldJPPTTs };
    const char* gltfAttribData{
      gltfAttribute->data->offset +
      gltfAttribute->data->buffer_view->offset +
      ( const char* )gltfAttribute->data->buffer_view->buffer->data
    };

    const char* gltfAttribEndData{
      gltfAttribData +
      ( gltfAttribute->data->count * gltfAttribute->data->stride ) };

    const MetaType* jpptMetaType{ &GetMetaType< T >() };
    const MetaType* gltfMetaType{};
    TAC_ASSERT( gltfAttribute->data->component_type == cgltf_component_type_r_32f );
    if( gltfAttribute->data->type == cgltf_type_vec2 ) { gltfMetaType = &GetMetaType< v2 >(); }
    if( gltfAttribute->data->type == cgltf_type_vec3 ) { gltfMetaType = &GetMetaType< v3 >(); }
    if( gltfAttribute->data->type == cgltf_type_vec4 ) { gltfMetaType = &GetMetaType< v4 >(); }
    TAC_ASSERT( gltfMetaType );

    while( gltfAttribData < gltfAttribEndData )
    {
      T jpptT;
      const MetaType::CastParams castParams
      {
        .mDst     { &jpptT },
        .mSrc     { gltfAttribData },
        .mSrcType { gltfMetaType },
      };
      jpptMetaType->Cast( castParams );
      *pJpptT++ = jpptT;
      gltfAttribData += gltfAttribute->data->stride;
    }

  }

  static void AppendCPUMeshData( dynmc JPPTCPUMeshData& jpptCPUMeshData,
                                 const cgltf_primitive* parsedPrim )
  {
    const int primIndexCount{ ( int )parsedPrim->indices->count };
    const int oldIndexCount{ jpptCPUMeshData.mIndexes.size() };
    jpptCPUMeshData.mIndexes.resize( oldIndexCount + primIndexCount );

    const MetaType* gltfIndexMetaType{
      FindMetaType_from_cgltf_component_type( parsedPrim->indices->component_type ) };
    TAC_ASSERT( gltfIndexMetaType );

    const MetaType* jpptIndexMetaType{ &GetMetaType< JPPTCPUMeshData::IndexType >() };
    TAC_ASSERT( jpptIndexMetaType );

    TAC_ASSERT( parsedPrim->type == cgltf_primitive_type_triangles ); // triangle list
    TAC_ASSERT( primIndexCount % 3 == 0 );
    TAC_ASSERT( parsedPrim->indices->type == cgltf_type_scalar );

    for( int ii{}; ii < primIndexCount; ++ii )
    {
      char* gltfIndex{
        parsedPrim->indices->offset + // offset into buffer view
        parsedPrim->indices->buffer_view->offset + // offset into buffer
        ( parsedPrim->indices->stride * ii ) + // stride (using accessor stride, not buffer view stride)
        ( char* )parsedPrim->indices->buffer_view->buffer->data }; // buffer start

      JPPTCPUMeshData::IndexType jpptIndex;
      const MetaType::CastParams castParams
      {
        .mDst     { &jpptIndex },
        .mSrc     { gltfIndex },
        .mSrcType { gltfIndexMetaType },
      };
      jpptIndexMetaType->Cast( castParams );

      jpptCPUMeshData.mIndexes[ ii + oldIndexCount ] = oldIndexCount + jpptIndex;
    }

    AppendCPUMeshAttribute( jpptCPUMeshData.mPositions, parsedPrim, Render::Attribute::Position );
    AppendCPUMeshAttribute( jpptCPUMeshData.mTexCoords, parsedPrim, Render::Attribute::Texcoord );
    AppendCPUMeshAttribute( jpptCPUMeshData.mNormals, parsedPrim, Render::Attribute::Normal );
  }

  static Vector< SubMesh > PopulateSubmeshes( const AssetPathStringView& path,
                                              const int specifiedMeshIndex,
                                              dynmc Render::VertexDeclarations& vtxDecls, // in/out
                                              dynmc MeshRaycast& meshRaycast, // appended to
                                              dynmc JPPTCPUMeshData& jpptCPUMeshData, // append to
                                              Errors& errors )
  {
    Vector< SubMesh > submeshes;
    LoadedGltfData loadedData;
    TAC_CALL_RET( loadedData.Load( path, errors ) );

    const cgltf_data* parsedData{ loadedData.parsedData };

    const int meshCount{ ( int )parsedData->meshes_count };
    TAC_ASSERT_INDEX( specifiedMeshIndex, meshCount );

    const cgltf_mesh* parsedMesh{ &parsedData->meshes[ specifiedMeshIndex ] };

    const int primitiveCount{ ( int )parsedMesh->primitives_count };
    for( int iPrim{}; iPrim < primitiveCount; ++iPrim )
    {
      const cgltf_primitive* parsedPrim{ &parsedMesh->primitives[ iPrim ] };
      if( !parsedPrim->attributes_count )
        continue;

      AppendCPUMeshData( jpptCPUMeshData, parsedPrim );

      if( vtxDecls.empty() )
        vtxDecls = GLTFVtxDecl( parsedPrim );
      TAC_ASSERT( parsedPrim->indices->type == cgltf_type_scalar );
      const String bufferNamePrefix{[&](){
        String bufferName;
        bufferName += '\"';
        bufferName += path.GetFilename();
        bufferName += '\"';
        if( !( meshCount == 1 && primitiveCount == 1 ) )
        {
          bufferName += ":";
          bufferName += ToString( specifiedMeshIndex );
          bufferName += ":";
          bufferName += ToString( iPrim );
        }
        return bufferName;
      }()};
      const String vtxBufName{ bufferNamePrefix + " vtxs" };
      const String idxBufName{ bufferNamePrefix + " idxs" };
      const int vertexCount{ ( int )parsedPrim->attributes[ 0 ].data->count };
      const int indexCount{ ( int )parsedPrim->indices->count };
      TAC_CALL_RET( const Render::BufferHandle indexBuffer{
        ConvertToIndexBuffer( parsedPrim, vtxBufName, errors ) } );
      TAC_CALL_RET( const Render::BufferHandle vertexBuffer{
        ConvertToVertexBuffer( vtxDecls, parsedPrim, idxBufName, errors ) } );

      const MeshRaycast subMeshRaycast{ GetMeshRaycast( parsedPrim ) };
      for( const MeshRaycast::SubMeshTriangle& tri : subMeshRaycast.mTris )
        meshRaycast.mTris.push_back( tri );

      const Render::PrimitiveTopology topology{ Render::PrimitiveTopology::TriangleList };
      const cgltf_primitive_type supportedType{ GetGltfFromTopology( topology ) };
      TAC_ASSERT( parsedPrim->type == supportedType );
      Render::IBindlessArray* bindlessArray{ ModelAssetManager::GetBindlessArray() };
      TAC_CALL_RET( const Render::IBindlessArray::Binding vtxBufBinding{
        bindlessArray->Bind( vertexBuffer, errors ) } );
      const SubMesh subMesh
      {
        .mPrimitiveTopology    { topology },
        .mVertexBuffer         { vertexBuffer },
        .mIndexBuffer          { indexBuffer },
        .mVertexBufferBinding  { vtxBufBinding },
        .mIndexCount           { indexCount },
        .mVertexCount          { vertexCount },
        .mName                 { StringView( bufferNamePrefix ) },
      };
      submeshes.push_back( subMesh );
    } // for each primitive

    TryGenerateCPUMeshNormals( jpptCPUMeshData );
    TryGenerateCPUMeshTangents( jpptCPUMeshData );

    return submeshes;
  }

  static Mesh                 LoadMeshFromGltf( ModelAssetManager::Params params,
                                                Errors& errors )
  {
    dynmc Render::VertexDeclarations vtxDecls{ params.mOptVtxDecls };
    dynmc MeshRaycast meshRaycast;
    dynmc JPPTCPUMeshData jpptCPUMeshData;
    TAC_CALL_RET( const Vector< SubMesh > submeshes{
      PopulateSubmeshes( params.mPath,
                         params.mModelIndex,
                         vtxDecls,
                         meshRaycast,
                         jpptCPUMeshData,
                         errors ) } );
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
      .mGPUInputLayoutBuffer  { gpuInputLayoutBuffer },
      .mGPUInputLayoutBinding { gpuInputLayoutBinding },
      .mMeshRaycast           { meshRaycast },
      .mJPPTCPUMeshData       { jpptCPUMeshData },
    };
  }

  void                        GltfLoaderInit()
  {
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".gltf" );
    ModelLoadFunctionRegister( LoadMeshFromGltf, ".glb" );
  }
} // namespace Tac
