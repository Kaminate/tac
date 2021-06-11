#include "src/common/assetmanagers/tacModelAssetManagerBackend.h"
#include "src/common/tacTemporaryMemory.h"

namespace Tac
{
  static Mesh LoadWavefrontObj( const char* path,
                                 int iModel,
                                 const Render::VertexDeclarations& vertexDeclarations,
                                 Errors& errors )
  {
    auto bytes = TemporaryMemoryFromFile( path, errors );

    Mesh mesh;
    return mesh;
  }

  static int sRegistration = []()
  {
    ModelLoadFunctionRegister( LoadWavefrontObj, "obj" );
    return 0;
  }( );


} // namespace Tac
