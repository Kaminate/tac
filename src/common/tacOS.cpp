#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacErrorHandling.h"
namespace Tac
{


OS* OS::Instance = nullptr;
void OS::CreateFolderIfNotExist( const String& path, Errors& errors )
{
  bool exist;
  DoesFolderExist( path, exist, errors );
  TAC_HANDLE_ERROR( errors );
  if( exist )
    return;
  CreateFolder( path, errors );
  TAC_HANDLE_ERROR( errors );
}

void OS::DebugAssert( const String& msg, const Frame& frame )
{
  String s = msg + "\n" + frame.ToString();
  if( !IsDebugMode() )
    return;
  std::cout << s << std::endl;
  DebugBreak();
  DebugPopupBox( s );
  exit( -1 );
}

}
