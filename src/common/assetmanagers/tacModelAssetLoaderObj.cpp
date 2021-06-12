#include "src/common/assetmanagers/tacModelAssetManagerBackend.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTextParser.h"
#include "src/common/math/tacMath.h"

namespace Tac
{
  struct WavefrontObjVertex
  {
    int miPosition = 0;
    int miNormal = 0;
    int miTexCoord = 0;
  };

  struct WavefrontObjFace
  {
    WavefrontObjVertex mVertexes[ 3 ] = {};
  };

  struct WavefrontObj
  {
    Vector< v3 > normals;
    Vector< v2 > texcoords;
    Vector< v3 > positions;
    Vector< WavefrontObjFace > faces;
  };

  static WavefrontObjVertex WavefrontObjParseVertex( StringView line )
  {
    static String lineCopy;
    lineCopy = line;
    for( char& c : lineCopy )
      c = c == '/' ? ' ' : c;

    ParseData parseData( lineCopy.begin(), lineCopy.end() );

    WavefrontObjVertex vertex;
    const int slashCount = Count( line, '/' );
    if( slashCount == 0 )
    {
      vertex.miPosition = ( int )parseData.EatFloat().GetValueUnchecked();
    }
    else if( slashCount == 1 )
    {
      vertex.miPosition = ( int )parseData.EatFloat().GetValueUnchecked();
      vertex.miTexCoord = ( int )parseData.EatFloat().GetValueUnchecked();
    }
    else if( slashCount == 2 )
    {
      if( line.find_first_of( "//" ) )
      {
        vertex.miPosition = ( int )parseData.EatFloat().GetValueUnchecked();
        vertex.miNormal = ( int )parseData.EatFloat().GetValueUnchecked();
      }
      else
      {
        vertex.miPosition = ( int )parseData.EatFloat().GetValueUnchecked();
        vertex.miTexCoord = ( int )parseData.EatFloat().GetValueUnchecked();
        vertex.miNormal = ( int )parseData.EatFloat().GetValueUnchecked();
      }

    }

    return vertex;
  }

  static WavefrontObjFace   WavefrontObjParseFace( StringView line )
  {
    ParseData parseData( line.begin(), line.end() );
    WavefrontObjFace face = {};
    int iVertex = 0;
    while( iVertex < 3 )
    {
      StringView vertexString = parseData.EatWord();
      if( vertexString.empty() )
        break;
      face.mVertexes[ iVertex++ ] = WavefrontObjParseVertex( vertexString );
    }

    return face;
  }

  static WavefrontObj       WavefrontObjLoad( const void* bytes, int byteCount )
  {
    ParseData parseData( ( const char* )bytes, byteCount );

    Vector< v3 > normals;
    Vector< v2 > texcoords;
    Vector< v3 > positions;
    Vector< WavefrontObjFace > faces;

    for( ;; )
    {
      if( parseData.GetRemainingByteCount() == 0 )
        break;

      const StringView word = parseData.EatWord();
      if( word == "f" )
        faces.push_back( WavefrontObjParseFace( parseData.EatRestOfLine() ) );

      if( word == "vn" )
        normals.push_back( v3( parseData.EatFloat().GetValueUnchecked(),
                               parseData.EatFloat().GetValueUnchecked(),
                               parseData.EatFloat().GetValueUnchecked() ) );

      if( word == "vt" )
        texcoords.push_back( v2( parseData.EatFloat().GetValueUnchecked(),
                                 parseData.EatFloat().GetValueUnchecked() ) );


      if( word == "v" )
        positions.push_back( v3( parseData.EatFloat().GetValueUnchecked(),
                                 parseData.EatFloat().GetValueUnchecked(),
                                 parseData.EatFloat().GetValueUnchecked() ) );

      parseData.EatRestOfLine();
    }

    WavefrontObj result;
    result.normals = normals;
    result.texcoords = texcoords;
    result.positions = positions;
    result.faces = faces;
    return result;
  }

  static Mesh               WavefrontObjConvertToMesh( const char* name,
                                                       const WavefrontObj& wavefrontObj,
                                                       const Render::VertexDeclarations& vertexDeclarations )
  {
    const int stride = [ & ]()
    {
      int stride = 0;
      for( auto decl : vertexDeclarations )
      {
        const int curStride = decl.mAlignedByteOffset + decl.mTextureFormat.CalculateTotalByteCount();
        stride = Max( stride, curStride );
      }
      return stride;
    }( );

    Vector< char > dstVtxBytes;
    Vector< char > dstIdxBytes;
    Vector< char > vertexBytes( stride );

    SubMeshTriangles subMeshTriangles;

    for( const WavefrontObjFace& face : wavefrontObj.faces )
    {
      SubMeshTriangle subMeshTriangle;
      for( int iTriVert = 0; iTriVert < 3; ++iTriVert )
      {
        const WavefrontObjVertex& tri = face.mVertexes[ iTriVert ];
        subMeshTriangle[ iTriVert ] = wavefrontObj.positions[ tri.miPosition ];

        MemSet( vertexBytes.data(), 0, vertexBytes.size() );

        for( auto decl : vertexDeclarations )
        {
          if( decl.mAttribute == Render::Attribute::Position && wavefrontObj.positions.size() )
          {
            TAC_ASSERT( decl.mTextureFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mTextureFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mTextureFormat.mPerElementDataType == Render::GraphicsType::real );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.positions[ tri.miPosition ].data(),
                    sizeof( v3 ) );

          }
          if( decl.mAttribute == Render::Attribute::Normal && wavefrontObj.normals.size() )
          {
            TAC_ASSERT( decl.mTextureFormat.mElementCount == 3 );
            TAC_ASSERT( decl.mTextureFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mTextureFormat.mPerElementDataType == Render::GraphicsType::real );
            MemCpy( vertexBytes.data() + decl.mAlignedByteOffset,
                    wavefrontObj.normals[ tri.miNormal ].data(),
                    sizeof( v3 ) );
          }
          if( decl.mAttribute == Render::Attribute::Texcoord && wavefrontObj.texcoords.size() )
          {
            TAC_ASSERT( decl.mTextureFormat.mElementCount == 2 );
            TAC_ASSERT( decl.mTextureFormat.mPerElementByteCount == sizeof( float ) );
            TAC_ASSERT( decl.mTextureFormat.mPerElementDataType == Render::GraphicsType::real );
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

    const Render::VertexBufferHandle vertexBuffer = Render::CreateVertexBuffer( dstVtxBytes.size(),
                                                                                dstVtxBytes.data(),
                                                                                stride,
                                                                                Render::Access::Default,
                                                                                TAC_STACK_FRAME );

    SubMesh subMesh;
    subMesh.mName = name;
    subMesh.mTris = subMeshTriangles;
    subMesh.mVertexBuffer = vertexBuffer;

    Mesh mesh;
    mesh.mSubMeshes = { subMesh };
    return mesh;
  }

  static Mesh               WavefrontObjLoadIntoMesh( const char* path,
                                                      int iModel,
                                                      const Render::VertexDeclarations& vertexDeclarations,
                                                      Errors& errors )
  {
    TAC_UNUSED_PARAMETER( iModel );
    const TemporaryMemory bytes = TemporaryMemoryFromFile( path, errors );
    const WavefrontObj wavefrontObj = WavefrontObjLoad( bytes.data(), bytes.size() );
    const Mesh mesh = WavefrontObjConvertToMesh( path, wavefrontObj, vertexDeclarations );
    return mesh;
  }

  static int sRegistration = []()
  {
    ModelLoadFunctionRegister( WavefrontObjLoadIntoMesh, "obj" );
    return 0;
  }( );


} // namespace Tac
