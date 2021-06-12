#include "src/common/assetmanagers/tacModelAssetManagerBackend.h"
#include "src/common/tacHash.h"
#include "src/common/tacUtility.h"

#include <map>

namespace Tac
{
  static std::map< HashedValue, MeshLoadFunction* > functionMap;
  //typedef std::map< HashedValue, MeshLoadFunction* > FunctionMap;
  //static FunctionMap sFunctionMap;
  //static FunctionMap& GetFunctionMap()
  //{
  //  return sFunctionMap;
  //}
  //static std::map< HashedValue, MeshLoadFunction* > sFunctionMap;
  
  static HashedValue ModelExtensionHash( const char* ext )
  {
    HashedValue hashedValue = 0;
    while( *ext )
    {
      hashedValue = HashAdd( hashedValue, ToLower( *ext ) );
      ext++;
    }
    return hashedValue ;
  }

  void               ModelLoadFunctionRegister( MeshLoadFunction* meshLoadFunction, const char* ext )
  {
    const HashedValue hashedValue = ModelExtensionHash( ext );
    //FunctionMap& functionMap = GetFunctionMap();
    functionMap[ hashedValue ] = meshLoadFunction;
  }

  MeshLoadFunction*  ModelLoadFunctionFind( const char* ext )
  {
    const HashedValue hashedValue = ModelExtensionHash( ext );
    //FunctionMap& functionMap = GetFunctionMap();
    const auto it = functionMap.find( hashedValue );
    return it == functionMap.end() ? nullptr : ( *it ).second;
  }
}

