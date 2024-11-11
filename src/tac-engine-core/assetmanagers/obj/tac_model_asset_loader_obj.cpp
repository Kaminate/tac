#include "tac_model_asset_loader_obj.h" // self-inc

#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager_backend.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{

  static WavefrontObj::Vertex WavefrontObjParseVertex( StringView line )
  {
    ParseData parseData( line.begin(), line.end() );
    WavefrontObj::Vertex vertex;
    const int slashCount = Count( line, '/' );
    if( slashCount == 0 )
    {
      vertex.miPosition = ( int )parseData.EatFloat().GetValue() - 1;
    }
    else if( slashCount == 1 )
    {
      vertex.miPosition = ( int )parseData.EatFloat().GetValue() - 1;
      parseData.EatStringExpected( "/" );
      vertex.miTexCoord = ( int )parseData.EatFloat().GetValue() - 1;
    }
    else if( slashCount == 2 )
    {
      vertex.miPosition = ( int )parseData.EatFloat().GetValue() - 1;
      parseData.EatStringExpected( "/" );
      if( line.find( "//" ) == line.npos )
        vertex.miTexCoord = ( int )parseData.EatFloat().GetValue() - 1;
      parseData.EatStringExpected( "/" );
      vertex.miNormal = ( int )parseData.EatFloat().GetValue() - 1;
    }
    return vertex;
  }

  static WavefrontObj::Face   WavefrontObjParseFace( ParseData* parseData )
  {
    //ParseData parseData( line.begin(), line.end() );
    WavefrontObj::Face face  {};
    for( WavefrontObj::Vertex* vertex { face.mVertexes }; vertex < face.mVertexes + 3; ++vertex )
      if( StringView vertexString { parseData->EatWord() }; !vertexString.empty() )
        *vertex = WavefrontObjParseVertex( vertexString );

    return face;
  }

  static v2 EatV2( ParseData* parseData )
  {
    float x { parseData->EatFloat().GetValue() };
    float y { parseData->EatFloat().GetValue() };
    return { x, y };
  }

  static v3 EatV3( ParseData* parseData )
  {
    float x { parseData->EatFloat().GetValue() };
    float y { parseData->EatFloat().GetValue() };
    float z { parseData->EatFloat().GetValue() };
    return { x, y, z };
  }

  WavefrontObj       WavefrontObj::Load( const void* bytes, int byteCount )
  {
    ParseData                  parseData( ( const char* )bytes, byteCount );
    Vector< v3 >               normals;
    Vector< v2 >               texcoords;
    Vector< v3 >               positions;
    Vector< Face > faces;

    for( ;; )
    {
      if( parseData.GetRemainingByteCount() == 0 )
        break;

      const StringView word{ parseData.EatWord() };
      if( word == StringView( "f" ) )
        faces.push_back( WavefrontObjParseFace( &parseData ) );
      else if( word == StringView( "vn" ) )
        normals.push_back( EatV3( &parseData ) );
      else if( word == StringView( "vt" ) )
        texcoords.push_back( EatV2( &parseData ) );
      else if( word == StringView( "v" ) )
        positions.push_back( EatV3( &parseData ) );
      else
      {
        TAC_ASSERT_UNIMPLEMENTED;
      }

      parseData.EatRestOfLine();
    }

    return WavefrontObj
    {
      .mNormals   { normals },
      .mTexCoords { texcoords },
      .mPositions { positions },
      .mFaces     { faces },
    };
  }


  static SubMeshTriangles   WavefrontObjGetSubMeshTriangles( const WavefrontObj& wavefrontObj )
  {
    SubMeshTriangles subMeshTriangles;

    for( const WavefrontObj::Face& face : wavefrontObj.mFaces )
    {
      SubMeshTriangle subMeshTriangle  {};
      for( int iTriVert {}; iTriVert < 3; ++iTriVert )
      {
        const WavefrontObj::Vertex& tri { face.mVertexes[ iTriVert ] };
        subMeshTriangle[ iTriVert ] = wavefrontObj.mPositions[ tri.miPosition ];
      }

      subMeshTriangles.push_back( subMeshTriangle );
    }

    return subMeshTriangles;
  }


  static Mesh               WavefrontObjConvertToMesh( const StringView& name,
                                                       const WavefrontObj& wavefrontObj,
                                                       const Render::VertexDeclarations& vertexDeclarations,
                                                       Errors& errors )
  {
    const int stride { vertexDeclarations.CalculateStride() };

    Vector< char > dstVtxBytes;
    Vector< char > dstIdxBytes;
    Vector< char > vertexBytes( stride );

    const SubMeshTriangles subMeshTriangles{ WavefrontObjGetSubMeshTriangles( wavefrontObj ) };

    for( const WavefrontObj::Face& face : wavefrontObj.mFaces )
    {
      for( int iTriVert {}; iTriVert < 3; ++iTriVert )
      {
        const WavefrontObj::Vertex& tri { face.mVertexes[ iTriVert ] };

        MemSet( vertexBytes.data(), 0, vertexBytes.size() );

        for( const Render::VertexDeclaration& decl : vertexDeclarations )
        {
          const int nPos{ wavefrontObj.mPositions.size() };
          if( decl.mAttribute == Render::Attribute::Position && nPos )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miPosition, wavefrontObj.mPositions.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.mPositions[ tri.miPosition ].data(),
                    sizeof( v3 ) );

          }

          const int nNor{ wavefrontObj.mNormals.size() };
          if( decl.mAttribute == Render::Attribute::Normal && nNor )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miNormal, wavefrontObj.mNormals.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.mNormals[ tri.miNormal ].data(),
                    sizeof( v3 ) );
          }

          const int nUV{ wavefrontObj.mTexCoords.size() };
          if( decl.mAttribute == Render::Attribute::Texcoord && nUV )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 2 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miTexCoord, wavefrontObj.mTexCoords.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.mTexCoords[ tri.miTexCoord ].data(),
                    sizeof( v2 ) );
          }
        }

        for( char c : vertexBytes )
          dstVtxBytes.push_back( c );
      }
    }

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::CreateBufferParams vertexBufferParams
    {
      .mByteCount    { dstVtxBytes.size() },
      .mBytes        { dstVtxBytes.data() },
      .mStride       { stride },
      .mUsage        { Render::Usage::Static },
      .mBinding      { Render::Binding::VertexBuffer },
      .mOptionalName { name },
      .mStackFrame   { TAC_STACK_FRAME },
    };
    TAC_CALL_RET( const Render::BufferHandle vertexBuffer{
       renderDevice->CreateBuffer( vertexBufferParams, errors ) } );

      const int vtxCount{ wavefrontObj.mFaces.size() * 3 };

    const SubMesh subMesh
    {
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mVertexBuffer      { vertexBuffer },
      .mTris              { subMeshTriangles },
      .mVertexCount       { vtxCount },
      .mName              { name },
    };

    return Mesh
    {
      .mSubMeshes   { subMesh },
      .mVertexDecls { vertexDeclarations },
    };
  }

  struct WavefrontObjVtxDeclCreator
  {
    static Render::VertexDeclarations Create( const WavefrontObj& wavefrontObj  )
    {
      return WavefrontObjVtxDeclCreator( wavefrontObj ).mVertexDeclarations;
    }

  private:
    WavefrontObjVtxDeclCreator( const WavefrontObj& wavefrontObj )
    {
      if( !wavefrontObj.mPositions.empty() )
        AddAttribute( Render::Attribute::Position, Render::VertexAttributeFormat::GetVector3() );

      if( !wavefrontObj.mNormals.empty() )
        AddAttribute( Render::Attribute::Normal, Render::VertexAttributeFormat::GetVector3() );

      if( !wavefrontObj.mTexCoords.empty() )
        AddAttribute( Render::Attribute::Texcoord, Render::VertexAttributeFormat::GetVector2() );

      TAC_ASSERT( mVertexDeclarations.CalculateStride() == mRunningStride );
    };

    void AddAttribute( Render::Attribute attrib, Render::VertexAttributeFormat vaf )
    {
        const Render::VertexDeclaration vtxDecl
        {
          .mAttribute         { attrib },
          .mFormat            { vaf },
          .mAlignedByteOffset { mRunningStride },
        };
        mVertexDeclarations.push_back( vtxDecl );
        mRunningStride += vaf.CalculateTotalByteCount();
    }

    int                        mRunningStride{};
    Render::VertexDeclarations mVertexDeclarations;
  };


  static Mesh               WavefrontObjLoadIntoMesh( ModelAssetManager::Params params,
                                                      Errors& errors )
  {

    const AssetPathStringView assetPath{ params.mPath };
    const int iModel{ params.mModelIndex };
    dynmc Render::VertexDeclarations vertexDeclarations{ params.mOptVtxDecls };

    const StringView name { assetPath.GetFilename() };
    const String bytes { LoadAssetPath( assetPath, errors ) };
    const WavefrontObj wavefrontObj { WavefrontObj::Load( bytes.data(), bytes.size() ) };
    if( vertexDeclarations.empty() )
      vertexDeclarations = WavefrontObjVtxDeclCreator::Create( wavefrontObj );

    return WavefrontObjConvertToMesh( name, wavefrontObj, vertexDeclarations, errors );
  }

  void                      WavefrontObj::Init()
  {
    ModelLoadFunctionRegister( WavefrontObjLoadIntoMesh, ".obj" );
  }

} // namespace Tac

