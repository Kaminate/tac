#pragma once

#include "common/tacString.h"
#include "common/containers/tacVector.h"
#include "common/math/tacVector2.h"

struct TacErrors;
struct TacStackFrame;

struct TacOS
{
  virtual ~TacOS() = default;
  virtual void SaveToFile( const TacString& path, void* bytes, int byteCount, TacErrors& errors ) = 0;

  // SDL doesn't have this functionality
  // Maybe we shouldn't and just rely on the folder already existing?
  virtual void DoesFolderExist( const TacString& path, bool& exists, TacErrors& errors ) = 0;
  virtual void CreateFolder( const TacString& path, TacErrors& errors ) = 0;
  void CreateFolderIfNotExist( const TacString& path, TacErrors& errors );

  virtual void DebugBreak() = 0;
  void DebugAssert( const TacString& msg, const TacStackFrame& stackFrame );
  virtual void DebugPopupBox( const TacString& ) = 0;

  // Gets the path where you can save files to, such as user configs
  virtual void GetApplicationDataPath( TacString& path, TacErrors& errors ) = 0;

  virtual void GetFileLastModifiedTime( time_t* time, const TacString& path, TacErrors& errors ) = 0;

  virtual void GetDirFilesRecursive( TacVector<TacString>&files, const TacString& dir, TacErrors& errors ) = 0;

  virtual void SaveDialog( TacString& path, const TacString& suggestedPath, TacErrors& errors ) {};

  // same as current dir
  virtual void GetWorkingDir( TacString& dir, TacErrors& errors ) {};


  // I don't think this function should exist.
  // If you are debugging, hit a breakpoint, and THEN call this function,
  // you're mouse is likely to have moved.
  // What should happen is that this gets cached during the message pump,
  // and accessed through TacMouseInput
  virtual void GetScreenspaceCursorPos( v2& pos, TacErrors& errors ) = 0;
  virtual void SetScreenspaceCursorPos( v2& pos, TacErrors& errors ) = 0;


  bool mShouldStopRunning = false;

  virtual TacString GetDefaultRendererName(){return "";};

  static TacOS* Instance;
};

