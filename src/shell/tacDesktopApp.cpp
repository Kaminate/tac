#include "tacDesktopApp.h"

#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/tacOS.h"
#include "common/tackeyboardinput.h" // temp

TacDesktopApp::TacDesktopApp()
{
   new TacShell;
}

TacDesktopApp::~TacDesktopApp()
{
  for( auto window : mMainWindows )
    delete window;
}
void TacDesktopApp::Loop( TacErrors& errors )
{
  for( ;; )
  {
    if( TacOS::Instance->mShouldStopRunning )
      break;

    Poll( errors );
    TAC_HANDLE_ERROR( errors );

    int windowCount = mMainWindows.size();
    int iWindow = 0;
    while( iWindow < windowCount )
    {
      TacDesktopWindow* window = mMainWindows[ iWindow ];
      if( window->mRequestDeletion )
      {
        mMainWindows[ iWindow ] = mMainWindows[ windowCount - 1 ];
        delete window;
        --windowCount;
        mMainWindows.pop_back();
      }
      else
      {
        ++iWindow;
      }
    }

    if( TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
    {
      static int i = 5;
      i++;
    }

    TacShell::Instance->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }
}

void TacDesktopApp::SpawnWindow( const TacWindowParams& windowParams, TacDesktopWindow** ppDesktopWindow, TacErrors& errors )
{
  TacDesktopWindow* desktopWindow;
  SpawnWindowAux( windowParams, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  TacRenderer::Instance->CreateWindowContext( desktopWindow, errors );

  //struct TacOnWindowResize : public TacEvent<>::Handler
  //{
  //  void HandleEvent() override
  //  {
  //    TacErrors errors;
  //    mRendererWindowData->OnResize( errors );
  //  }
  //  TacRendererWindowData* mRendererWindowData = nullptr;
  //  TacDesktopWindow* mDesktopWindow = nullptr;
  //};
  //auto onWindowResize = new TacOnWindowResize;
  //onWindowResize->mDesktopWindow = desktopWindow;
  //onWindowResize->mRendererWindowData = desktopWindow->mRendererData;

  desktopWindow->mOnResize.AddCallbackFunctional( [desktopWindow, &errors]() {
    desktopWindow->mRendererData->OnResize( errors ); } );


  TAC_HANDLE_ERROR( errors );

  mMainWindows.push_back( desktopWindow );

  *ppDesktopWindow = desktopWindow;
}

