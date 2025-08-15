#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/filesystem/tac_filesystem.h" // FileSys::Path

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

    auto Run( Errors& ) -> FileSys::Path;

  private:
    void SetDefaultFolder( Errors& );
    void CreateDialogInstance( Errors& );
    void Show( Errors& );
    auto GetResult( Errors& ) -> FileSys::Path;

    IFileDialog*            mDialog     {};
    PCom< IFileOpenDialog > mOpenDialog {};
    PCom< IFileSaveDialog > mSaveDialog {};
    bool                    mCancelled  {};
    Type                    mType       {};
  };

} // namespace Tac

