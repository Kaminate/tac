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
  struct WavefrontObjVertex
  {
    int miPosition {};
    int miNormal {};
    int miTexCoord {};
  };

  struct WavefrontObjFace
  {
    WavefrontObjVertex mVertexes[ 3 ]  {};
  };

  struct WavefrontObj
  {
    Vector< v3 >               normals;
    Vector< v2 >               texcoords;
    Vector< v3 >               positions;
    Vector< WavefrontObjFace > faces;
  };

  static WavefrontObjVertex WavefrontObjParseVertex( StringView line )
  {
    ParseData parseData( line.begin(), line.end() );
    WavefrontObjVertex vertex;
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

  static WavefrontObjFace   WavefrontObjParseFace( ParseData* parseData )
  {
    //ParseData parseData( line.begin(), line.end() );
    WavefrontObjFace face  {};
    for( WavefrontObjVertex* vertex = face.mVertexes; vertex < face.mVertexes + 3; ++vertex )
    {
      StringView vertexString { parseData->EatWord() };
      if( vertexString.empty() )
        break;
      *vertex = WavefrontObjParseVertex( vertexString );
    }
    return face;
  }

  v2 EatV2( ParseData* parseData )
  {
    float x { parseData->EatFloat().GetValue() };
    float y { parseData->EatFloat().GetValue() };
    return { x, y };
  }

  v3 EatV3( ParseData* parseData )
  {
    float x { parseData->EatFloat().GetValue() };
    float y { parseData->EatFloat().GetValue() };
    float z { parseData->EatFloat().GetValue() };
    return { x, y, z };
  }

  static WavefrontObj       WavefrontObjLoad( const void* bytes, int byteCount )
  {
    ParseData                  parseData( ( const char* )bytes, byteCount );
    Vector< v3 >               normals;
    Vector< v2 >               texcoords;
    Vector< v3 >               positions;
    Vector< WavefrontObjFace > faces;

    for( ;; )
    {
      if( parseData.GetRemainingByteCount() == 0 )
        break;

      const StringView word { parseData.EatWord() };
      if( word == StringView("f" ) )
        faces.push_back( WavefrontObjParseFace( &parseData ) );

      if( word == StringView("vn") )
        normals.push_back( EatV3( &parseData ) );

      if( word == StringView("vt") )
        texcoords.push_back( EatV2( &parseData ) );

      if( word == StringView("v") )
        positions.push_back( EatV3( &parseData ) );

      parseData.EatRestOfLine();
    }

    return WavefrontObj{
      .normals { normals },
      .texcoords { texcoords },
      .positions { positions },
      .faces { faces },
    };
  }

  static int                WavefrontObjCalculateStride( const Render::VertexDeclarations& vertexDeclarations )
  {
    int maxStride {};
    for( const Render::VertexDeclaration& decl : vertexDeclarations )
    {
      const int curStride { decl.mAlignedByteOffset + decl.mFormat.CalculateTotalByteCount() };
      maxStride = Max( maxStride, curStride );
    }

    return maxStride;
  }

  static Mesh               WavefrontObjConvertToMesh( const StringView& name,
                                                       const WavefrontObj& wavefrontObj,
                                                       const Render::VertexDeclarations& vertexDeclarations,
                                                       Errors& errors )
  {
    const int stride { WavefrontObjCalculateStride( vertexDeclarations ) };

    Vector< char > dstVtxBytes;
    Vector< char > dstIdxBytes;
    Vector< char > vertexBytes( stride );

    SubMeshTriangles subMeshTriangles;

    for( const WavefrontObjFace& face : wavefrontObj.faces )
    {
      SubMeshTriangle subMeshTriangle  {};
      for( int iTriVert {}; iTriVert < 3; ++iTriVert )
      {
        const WavefrontObjVertex& tri { face.mVertexes[ iTriVert ] };
        subMeshTriangle[ iTriVert ] = wavefrontObj.positions[ tri.miPosition ];

        MemSet( vertexBytes.data(), 0, vertexBytes.size() );

        for( const Render::VertexDeclaration& decl : vertexDeclarations )
        {
          if( decl.mAttribute == Render::Attribute::Position && wavefrontObj.positions.size() )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miPosition, wavefrontObj.positions.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.positions[ tri.miPosition ].data(),
                    sizeof( v3 ) );

          }
          if( decl.mAttribute == Render::Attribute::Normal && wavefrontObj.normals.size() )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miNormal, wavefrontObj.normals.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.normals[ tri.miNormal ].data(),
                    sizeof( v3 ) );
          }
          if( decl.mAttribute == Render::Attribute::Texcoord && wavefrontObj.texcoords.size() )
          {
            TAC_ASSERT( decl.mFormat.mElementCount == 2 );
            TAC_ASSERT( decl.mFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mFormat.mPerElementDataType == Render::GraphicsType::real );
            TAC_ASSERT_INDEX( tri.miTexCoord, wavefrontObj.texcoords.size() );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.texcoords[ tri.miTexCoord ].data(),
                    sizeof( v2 ) );
          }
        }

        for( char c : vertexBytes )
          dstVtxBytes.push_back( c );
      }

      subMeshTriangles.push_back( subMeshTriangle );
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
    TAC_CALL_RET( {}, const Render::BufferHandle vertexBuffer{
       renderDevice->CreateBuffer( vertexBufferParams, errors ) } );

    const SubMesh subMesh
    {
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mVertexBuffer      { vertexBuffer },
      .mTris              { subMeshTriangles },
      .mVertexCount       { wavefrontObj.faces.size() * 3 },
      .mName              { name },
    };

    return Mesh{ .mSubMeshes { subMesh } };
  }

  static Mesh               WavefrontObjLoadIntoMesh( const AssetPathStringView& assetPath,
                                                      [[maybe_unused]] const int iModel,
                                                      const Render::VertexDeclarations& vertexDeclarations,
                                                      Errors& errors )
  {
    const StringView name { assetPath.GetFilename() };
    const String bytes { LoadAssetPath( assetPath, errors ) };
    const WavefrontObj wavefrontObj { WavefrontObjLoad( bytes.data(), bytes.size() ) };
    const Mesh mesh { WavefrontObjConvertToMesh( name, wavefrontObj, vertexDeclarations, errors ) };
    return mesh;
  }

  void                      WavefrontObjLoaderInit()
  {
    ModelLoadFunctionRegister( WavefrontObjLoadIntoMesh, ".obj" );
  }

} // namespace Tac

