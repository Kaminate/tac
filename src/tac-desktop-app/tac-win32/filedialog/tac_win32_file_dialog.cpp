#include "tac_win32_file_dialog.h" // self-inc

#include "tac-engine-core/shell/tac_shell.h" // sShellInitialWorkingDir
#include "tac-std-lib/error/tac_error_handling.h" // Errors
#include "tac-win32/tac_win32.h" // TAC_HR_CALL
#include "tac-win32/tac_win32_com_ptr.h" // PCom

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <string>
  #include <filesystem>
#endif

#include <shobjidl_core.h> // IFileDialog, IFileSaveDialog, IFileOpenDialog

namespace Tac
{

  struct FileDialogHelper
  {
    ~FileDialogHelper();
    void SetDefaultFolder( UTF8Path, Errors& );
    void InitDialog( REFCLSID, REFIID, void**, Errors& );
    void InitOpenDialog( Errors& );
    void InitSaveDialog( Errors& );
    auto RunEnd( Errors& ) -> UTF8Path;

    IFileDialog*            mDialog     {};
    PCom< IFileOpenDialog > mOpenDialog {};
    PCom< IFileSaveDialog > mSaveDialog {};
    bool                    mCancelled  {};
  };

  FileDialogHelper::~FileDialogHelper()
  {
    // CoUninitialize does bad things if we dont first release our iunknowns
    mOpenDialog = {};
    mSaveDialog = {};
    CoUninitialize();
  }

  auto Win32FileDialogSave( const SaveParams& params, Errors& errors ) -> UTF8Path
  {
    FileDialogHelper helper;
    TAC_CALL_RET( helper.InitSaveDialog( errors ) );
    TAC_CALL_RET( helper.SetDefaultFolder( params.mDefaultFolder, errors ) );
    return helper.RunEnd( errors );
  }

  auto Win32FileDialogOpen( const OpenParams& params, Errors& errors ) -> UTF8Path
  {
    FileDialogHelper helper;
    TAC_CALL_RET( helper.InitOpenDialog( errors ) );
    TAC_CALL_RET( helper.SetDefaultFolder( params.mDefaultFolder, errors ) );
    return helper.RunEnd( errors );
  }

  auto FileDialogHelper::RunEnd( Errors& errors ) -> UTF8Path
  {
    const HRESULT showResult{ mDialog->Show( nullptr ) };
    if( showResult == HRESULT_FROM_WIN32( ERROR_CANCELLED ) )
      return {};
    TAC_RAISE_ERROR_IF_RETURN( FAILED( showResult ), "Failed to show dialog" );
    PCom< IShellItem > pItem;
    TAC_HR_CALL_RET( mDialog->GetResult( pItem.CreateAddress() ) );
    PWSTR pszFilePath;
    TAC_HR_CALL_RET( pItem->GetDisplayName( SIGDN_FILESYSPATH, &pszFilePath ) );
    TAC_ON_DESTRUCT( CoTaskMemFree( pszFilePath ) );
    const std::filesystem::path stdPath( pszFilePath );
    return (char*)stdPath.u8string().c_str();
  }

  void FileDialogHelper::SetDefaultFolder( UTF8Path dir, Errors& errors )
  {
    if( dir.empty() )
      return;
    const std::filesystem::path stdDir( ( char8_t* )dir.c_str() );
    const std::wstring wDir { stdDir.wstring() };
    PCom< IShellItem > shDir;
    TAC_HR_CALL( SHCreateItemFromParsingName( wDir.c_str(), NULL, shDir.iid(), shDir.ppv() ) );
    TAC_HR_CALL( mDialog->SetDefaultFolder( ( IShellItem* )shDir ) );
  }

  void FileDialogHelper::InitDialog( REFCLSID sid, REFIID iid, void** ppv, Errors& errors )
  {
    TAC_HR_CALL( CoInitializeEx( NULL, COINIT_MULTITHREADED ) );
    TAC_HR_CALL( CoCreateInstance( sid, NULL, CLSCTX_INPROC_SERVER, iid, ppv ) );
  }

  void FileDialogHelper::InitOpenDialog( Errors& errors )
  {
    InitDialog( CLSID_FileOpenDialog, mOpenDialog.iid(), mOpenDialog.ppv(), errors );
    mDialog = mOpenDialog.Get();
  }

  void FileDialogHelper::InitSaveDialog( Errors& errors )
  {
    InitDialog( CLSID_FileSaveDialog, mSaveDialog.iid(), mSaveDialog.ppv(), errors );
    mDialog = mSaveDialog.Get();
  }

} // namespace Tac

