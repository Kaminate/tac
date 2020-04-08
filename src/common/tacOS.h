#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"

namespace Tac
{


struct Errors;
struct StackFrame;

struct OS
{
  virtual ~OS() = default;
  virtual void SaveToFile( const String& path, void* bytes, int byteCount, Errors& errors ) = 0;

  // SDL doesn't have this functionality
  // Maybe we shouldn't and just rely on the folder already existing?
  virtual void DoesFolderExist( const String& path, bool& exists, Errors& errors ) = 0;
  virtual void CreateFolder( const String& path, Errors& errors ) = 0;
  void CreateFolderIfNotExist( const String& path, Errors& errors );

  virtual void DebugBreak() = 0;
  void DebugAssert( const String& msg, const StackFrame& frame );
  virtual void DebugPopupBox( const String& ) = 0;

  // Gets the path where you can save files to, such as user configs
  virtual void GetApplicationDataPath( String& path, Errors& errors ) = 0;

  virtual void GetFileLastModifiedTime( time_t* time, const String& path, Errors& errors ) = 0;

  virtual void GetDirFilesRecursive( Vector<String>&files, const String& dir, Errors& errors ) = 0;

  virtual void SaveDialog( String& path, const String& suggestedPath, Errors& errors ) {};
  virtual void OpenDialog( String& path, Errors& errors ) {};

  // same as current dir
  virtual void GetWorkingDir( String& dir, Errors& errors ) {};


  // I don't think this function should exist.
  // If you are debugging, hit a breakpoint, and THEN call this function,
  // you're mouse is likely to have moved.
  // What should happen is that this gets cached during the message pump,
  // and accessed through MouseInput
  virtual void GetScreenspaceCursorPos( v2& pos, Errors& errors ) = 0;
  virtual void SetScreenspaceCursorPos( v2& pos, Errors& errors ) = 0;


  bool mShouldStopRunning = false;

  virtual String GetDefaultRendererName(){return "";};

  static OS* Instance;
};

}
