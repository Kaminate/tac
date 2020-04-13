#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"

namespace Tac
{


  struct Errors;
  struct StackFrame;


  namespace OS
  {
    void SaveToFile( const String& path, void* bytes, int byteCount, Errors& errors );

    // SDL doesn't have this functionality
    // Maybe we shouldn't and just rely on the folder already existing?
    void DoesFolderExist( const String& path, bool& exists, Errors& errors );
    void CreateFolder( const String& path, Errors& errors );
    void CreateFolderIfNotExist( const String& path, Errors& errors );

    void DebugBreak();
    void DebugAssert( const String& msg, const StackFrame& frame );
    void DebugPopupBox( const String& );

    // Gets the path where you can save files to, such as user configs
    void GetApplicationDataPath( String& path, Errors& errors );

    void GetFileLastModifiedTime( time_t* time, const String& path, Errors& errors );

    void GetDirFilesRecursive( Vector<String>&files, const String& dir, Errors& errors );

    void SaveDialog( String& path, const String& suggestedPath, Errors& errors );
    void OpenDialog( String& path, Errors& errors );

    // same as current dir
    void GetWorkingDir( String& dir, Errors& errors );


    // I don't think this function should exist.
    // If you are debugging, hit a breakpoint, and THEN call this function,
    // you're mouse is likely to have moved.
    // What should happen is that this gets cached during the message pump,
    // and accessed through MouseInput
    void GetScreenspaceCursorPos( v2& pos, Errors& errors );
    void SetScreenspaceCursorPos( v2& pos, Errors& errors );

    extern bool mShouldStopRunning;

    String GetDefaultRendererName();
  };

  // semaphore values cannot go less than 0,
  // so if the semaphore is currently at 0, it needs to wait for
  // someone to increment the semaphore first.
  namespace Semaphore
  {
    struct Handle
    {
      int mIndex = -1;
    };

    Handle Create();
    void WaitAndDecrement( Handle );
    void Increment( Handle ); // aka post
  }

}
