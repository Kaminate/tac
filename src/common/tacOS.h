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

  virtual void GetDirFilesRecrusive( TacVector<TacString>&files, const TacString& dir, TacErrors& errors ) = 0;

  virtual void GetScreenspaceCursorPos( v2& pos, TacErrors& errors ) = 0;


  bool mShouldStopRunning = false;

  virtual TacString GetDefaultRendererName(){return "";};

  static TacOS* Instance;
};

