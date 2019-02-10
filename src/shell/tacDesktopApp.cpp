#include "tacDesktopApp.h"

#include "common/tacRenderer.h"
//#include "common/tacTime.h"
#include "common/tacUI2D.h"
#include "common/tacUI.h"
#include "common/tacOS.h"

TacDesktopApp::TacDesktopApp()
{
  mShell = new TacShell();
}
void TacDesktopApp::Loop( TacErrors& errors )
{
  for( ;; )
  {
    if( TacOS::Instance->mShouldStopRunning )
      break;

    Poll( errors );
    TAC_HANDLE_ERROR( errors );

    mShell->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }
}

// toDO: pass windowParams by const ref
void TacDesktopApp::SpawnWindowOuter( const TacWindowParams& windowParams, TacDesktopWindow** ppDesktopWindow, TacErrors& errors )
{
  TacDesktopWindow* desktopWindow;
  SpawnWindow( windowParams, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  auto renderView = new TacRenderView();

  //auto ui2DDrawData = new TacUI2DDrawData();
  //ui2DDrawData->mUI2DCommonData = mShell->mUI2DCommonData;
  //ui2DDrawData->mRenderView = renderView;
  //TAC_HANDLE_ERROR( errors );

  //auto uiRoot = new TacUIRoot();
  //uiRoot->mKeyboardInput = mShell->mKeyboardInput;
  //uiRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  //uiRoot->mUI2DDrawData = ui2DDrawData;
  //uiRoot->mDesktopWindow = desktopWindow;

  desktopWindow->mRenderView = renderView;
  //desktopWindow->mUI2DDrawData = ui2DDrawData;
  //desktopWindow->mUIRoot = uiRoot;

  mShell->mRenderer->CreateWindowContext( desktopWindow, errors );


  // subscribe graphics system to window creation?
  struct TacOnWindowResize : public TacEvent<>::Handler
  {
    void HandleEvent() override
    {
      TacErrors errors;
      mRendererWindowData->OnResize( errors );
    }
    TacRendererWindowData* mRendererWindowData = nullptr;
    TacDesktopWindow* mDesktopWindow = nullptr;
  };
  auto onWindowResize = new TacOnWindowResize;
  onWindowResize->mDesktopWindow = desktopWindow;
  onWindowResize->mRendererWindowData = desktopWindow->mRendererData;

  desktopWindow->mOnResize.AddCallback( onWindowResize );


  TAC_HANDLE_ERROR( errors );

  mMainWindows.push_back( desktopWindow );

  *ppDesktopWindow = desktopWindow;
}

