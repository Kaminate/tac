#include "tac_win32_file_dialog.h" // self-inc

#include "tac-std-lib/filesystem/tac_asset.h" // AssetPathRootFolderName
#include "tac-std-lib/error/tac_error_handling.h" // Errors
#include "tac-engine-core/shell/tac_shell.h" // sShellInitialWorkingDir
#include "tac-win32/tac_win32.h" // TAC_HR_CALL


#if 0
#include <Shlobj.h> // SHGetKnownFolderPath
#endif

namespace Tac
{
    FileDialogHelper::FileDialogHelper( Type type ) : mType( type ) { }

    FileDialogHelper::~FileDialogHelper()
    {
      // CoUninitialize does bad things if we dont first release our iunknowns
      mOpenDialog = {};
      mSaveDialog = {};
      CoUninitialize();
    }

    Filesystem::Path FileDialogHelper::Run( Errors& errors )
    {
      TAC_HR_CALL_RET( {}, CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ) );
      TAC_CALL_RET( {}, CreateDialogInstance( errors ) );
      TAC_CALL_RET( {}, SetDefaultFolder( errors ) );
      TAC_CALL_RET( {}, Show( errors ) );
      if( mCancelled )
        return {};
      return GetResult( errors );
    }

    void FileDialogHelper::SetDefaultFolder( Errors& errors )
    {
      const Filesystem::Path dir = sShellInitialWorkingDir / AssetPathRootFolderName;
      const std::wstring wDir = dir.Get().wstring();

      PCom<IShellItem> shDir;
      TAC_HR_CALL( SHCreateItemFromParsingName(
                   wDir.c_str(),
                   NULL,
                   shDir.iid(),
                   shDir.ppv() ) );
      TAC_HR_CALL( mDialog->SetDefaultFolder( ( IShellItem* )shDir ) );
    }

    void FileDialogHelper::CreateDialogInstance( Errors& errors )
    {
      REFCLSID sid = mType == Type::kOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
      REFIID iid = mType == Type::kOpen ? mOpenDialog.iid() : mSaveDialog.iid();
      void** ppv = mType == Type::kOpen ? mOpenDialog.ppv() : mSaveDialog.ppv();
      TAC_HR_CALL( CoCreateInstance( sid, NULL, CLSCTX_INPROC_SERVER, iid, ppv ) );
      mDialog = mType == Type::kOpen ? (IFileDialog*)mOpenDialog : (IFileDialog*)mSaveDialog;
    };

    void FileDialogHelper::Show( Errors& errors )
    {
      const HRESULT hr { mDialog->Show( nullptr ) };
      mCancelled = hr == HRESULT_FROM_WIN32( ERROR_CANCELLED );
      TAC_RAISE_ERROR_IF( FAILED( hr ) && !mCancelled, "Failed to show dialog");
    }

    Filesystem::Path FileDialogHelper::GetResult( Errors& errors )
    {
      PCom<IShellItem> pItem;
      TAC_HR_CALL_RET( {}, mDialog->GetResult( pItem.CreateAddress() ) );

      PWSTR pszFilePath;
      TAC_HR_CALL_RET( {}, pItem->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath ) );
      TAC_ON_DESTRUCT( CoTaskMemFree( pszFilePath ) );

      return std::filesystem::path( pszFilePath );
    }


}

