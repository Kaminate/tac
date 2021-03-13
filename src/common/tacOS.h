#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  struct Errors;
  struct StackFrame;

  namespace OS
  {
    void        SaveToFile( StringView path, void* bytes, int byteCount, Errors& );

    // SDL doesn't have this functionality
    // Maybe we shouldn't and just rely on the folder already existing?
    void        DoesFolderExist( StringView path, bool& exists, Errors& );
    void        CreateFolder( StringView path, Errors& );
    void        CreateFolderIfNotExist( StringView path, Errors& );

    void        DebugBreak();
    void        DebugAssert( const Errors& );
    void        DebugPopupBox( StringView );

    // Gets the path where you can save files to, such as user configs
    void        GetApplicationDataPath( String& path, Errors& );

    void        GetFileLastModifiedTime( time_t*, StringView path, Errors& );

    enum class  GetFilesInDirectoryFlags { Default = 0, Recursive = 1 };
    void        GetFilesInDirectory( Vector< String >& files, StringView dir, GetFilesInDirectoryFlags, Errors& );
    void        GetDirectoriesInDirectory( Vector< String >& dirs, StringView dir, Errors& );

    void        SaveDialog( String& path, StringView suggestedPath, Errors& );
    void        OpenDialog( String& path, Errors& );

    //          same as current dir
    void        GetWorkingDir( String& dir, Errors& );

    void        GetPrimaryMonitor( int* w, int* h );
  

    // I don't think this function should exist.
    // If you are debugging, hit a breakpoint, and THEN call this function,
    // you're mouse is likely to have moved.
    // What should happen is that this gets cached during the message pump,
    // and accessed through MouseInput
    void        GetScreenspaceCursorPos( v2& , Errors& );
    void        SetScreenspaceCursorPos( const v2& , Errors& );

    void        ThreadSleepSec( float );
    void        ThreadSleepMsec( int );

    bool        IsRunning();
    void        StopRunning();

  };

  // semaphore values cannot go less than 0,
  // so if the semaphore is currently at 0, it needs to wait for
  // someone to increment the semaphore first.
  TAC_DEFINE_HANDLE( SemaphoreHandle );
  SemaphoreHandle SemaphoreCreate();
  void            SemaphoreDecrementWait( SemaphoreHandle );
  void            SemaphoreIncrementPost( SemaphoreHandle );

}
