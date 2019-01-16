#include "tacOS.h"
#include "tacPreprocessor.h"
#include "tacErrorHandling.h"

TacOS* TacOS::Instance = nullptr;
void TacOS::CreateFolderIfNotExist( const TacString& path, TacErrors& errors )
{
  bool exist;
  DoesFolderExist( path, exist, errors );
  TAC_HANDLE_ERROR( errors );
  if( exist )
    return;
  CreateFolder( path, errors );
  TAC_HANDLE_ERROR( errors );
}

void TacOS::DebugAssert( const TacString& msg, const TacStackFrame& stackFrame )
{
  TacString s = msg + "\n" + stackFrame.ToString();
  if( !TacIsDebugMode() )
    return;
  std::cout << s << std::endl;
  DebugBreak();
  DebugPopupBox( s );
  exit( -1 );
}

