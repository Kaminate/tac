#pragma once

#include "src/common/string/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  struct Errors;
  struct StackFrame;

  void        OSSaveToFile( StringView path, void* bytes, int byteCount, Errors& );

  // SDL doesn't have this functionality
  // Maybe we shouldn't and just rely on the folder already existing?
  void        OSDoesFolderExist( StringView path, bool& exists, Errors& );
  void        OSCreateFolder( StringView path, Errors& );
  void        OSCreateFolderIfNotExist( StringView path, Errors& );

  void        OSDebugBreak();
  void        OSDebugAssert( const Errors& );
  void        OSDebugPopupBox( StringView );

  // Gets the path where you can save files to, such as user configs
  void        OSGetApplicationDataPath( String& path, Errors& );

  void        OSGetFileLastModifiedTime( time_t*, StringView path, Errors& );

  enum class  OSGetFilesInDirectoryFlags { Default = 0, Recursive = 1 };
  void        OSGetFilesInDirectory( Vector< String >& files, StringView dir, OSGetFilesInDirectoryFlags, Errors& );
  void        OSGetDirectoriesInDirectory( Vector< String >& dirs, StringView dir, Errors& );

  void        OSSaveDialog( String& path, StringView suggestedPath, Errors& );
  void        OSOpenDialog( String& path, Errors& );

  //          same as current dir
  void        OSGetWorkingDir( String& dir, Errors& );

  void        OSGetPrimaryMonitor( int* w, int* h );


  // I don't think this function should exist.
  // If you are debugging, hit a breakpoint, and THEN call this function,
  // you're mouse is likely to have moved.
  // What should happen is that this gets cached during the message pump,
  // and accessed through MouseInput
  //void        OSGetScreenspaceCursorPos( v2&, Errors& );

  // but like.. isnt it weird that Set exists and Get doesnt?
  void        OSSetScreenspaceCursorPos( const v2&, Errors& );

  void        OSThreadSleepSec( float );
  void        OSThreadSleepMsec( int );

  bool        OSAppIsRunning();
  void        OSAppStopRunning();


  // semaphore values cannot go less than 0,
  // so if the semaphore is currently at 0, it needs to wait for
  // someone to increment the semaphore first.
  TAC_DEFINE_HANDLE( SemaphoreHandle );
  SemaphoreHandle OSSemaphoreCreate();
  void            OSSemaphoreDecrementWait( SemaphoreHandle );
  void            OSSemaphoreIncrementPost( SemaphoreHandle );

}
