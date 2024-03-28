#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/filesystem/tac_filesystem.h" // Filesystem::Path

#include <shobjidl_core.h> // IFileDialog, IFileSaveDialog, IFileOpenDialog

namespace Tac
{
  struct Errors;

  struct FileDialogHelper
  {
    enum Type
    {
      kOpen,
      kSave,
    };

    FileDialogHelper( Type );
    ~FileDialogHelper();

    Filesystem::Path        Run( Errors& );

  private:
    void                    SetDefaultFolder( Errors& );
    void                    CreateDialogInstance( Errors& );
    void                    Show( Errors& );
    Filesystem::Path        GetResult( Errors& );

    IFileDialog*            mDialog = nullptr;
    PCom< IFileOpenDialog > mOpenDialog;
    PCom< IFileSaveDialog > mSaveDialog;
    bool                    mCancelled = false;
    Type                    mType;
  };

}
