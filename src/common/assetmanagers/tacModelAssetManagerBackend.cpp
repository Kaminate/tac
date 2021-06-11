#include "src/common/assetmanagers/tacModelAssetManagerBackend.h"
#include "src/common/tacHash.h"
#include "src/common/tacUtility.h"

#include <map>

namespace Tac
{
  std::map< HashedValue, MeshLoadFunction* > sFunctionMap;
  
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
    sFunctionMap[ hashedValue ] = meshLoadFunction;
  }

  MeshLoadFunction*  ModelLoadFunctionFind( const char* ext )
  {
    const HashedValue hashedValue = ModelExtensionHash( ext );
    const auto it = sFunctionMap.find( hashedValue );
    if( it == sFunctionMap.end() )
      return nullptr;
    return ( *it ).second;
  }
}

