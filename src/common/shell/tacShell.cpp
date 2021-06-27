#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacLog.h"
#include "src/common/tacNet.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacSettings.h"
#include "src/common/shell/tacShell.h"
#include "src/common/tacControllerinput.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/string/tacString.h"
#include "src/common/shell/tacShellTimer.h"

#include <iostream>
#include <thread>

namespace Tac
{
  const Key ToggleMainMenuKey = Key::Backtick;
  static String sAppName;
  static String sPrefPath;
  static String sInitialWorkingDir;

  Soul::Soul()
  {
    mIsImGuiVisible = true;
  }

  void            ShellUninit()
  {
    UI2DCommonDataUninit();
    Debug3DCommonDataUninit();

    gFontStuff.Uninit();

    //delete mLog;

    ModelAssetManagerUninit();

    // last, so resources can be freed
    Render::Uninit();
  }

  void            ShellInit( Errors& errors )
  {
    JobQueueInit();

    ModelAssetManagerInit();

    LocalizationLoad( "assets/localization.txt", errors );
    TAC_HANDLE_ERROR( errors );

    UI2DCommonDataInit( errors );
    TAC_HANDLE_ERROR( errors );

    Debug3DCommonDataInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void            ShellSetAppName( const char* s )           { sAppName = s;                      }
  const char*     ShellGetAppName()                          { return sAppName.c_str();           }
  void            ShellSetPrefPath( const char* s )          { sPrefPath = s;                     }
  const char*     ShellGetPrefPath()                         { return sPrefPath.c_str();          }
  void            ShellSetInitialWorkingDir( const char* s ) { sInitialWorkingDir = s;            }
  const char*     ShellGetInitialWorkingDir()                { return sInitialWorkingDir.c_str(); }

}
