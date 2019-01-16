#include "common/tacShell.h"
#include "common/tacPreprocessor.h"
#include "common/tacLog.h"
#include "common/tacRenderer.h"
#include "common/tacFont.h"
#include "common/tacNet.h"
#include "common/tacAlgorithm.h"
#include "common/taccontrollerinput.h"
#include "common/tacOS.h"
#include "common/tacTime.h"
#include "common/tacJobQueue.h"
#include "common/tacAssetManager.h"
#include "common/tacTextureAssetManager.h"
#include "common/imgui.h"
#include "common/tacSettings.h"
#include "common/tackeyboardinput.h"

#include <iostream>

const TacKey TacToggleMainMenuKey = TacKey::Backtick;


TacSoul::TacSoul()
{
  mIsImGuiVisible = true;
}
TacString TacSoul::GetDebugName()
{
  TacString result = "Soul " + TacToString( mID );
  return result;
}

TacShell::TacShell()
{
  mTimer = new TacTimer();
  mKeyboardInput = new TacKeyboardInput();
  mImGuiRender = false;// TacIsDebugMode();
  //mPaused = true;
  mTimer->Start();
  if( TacIsDebugMode() )
    mLog = new TacLog();
}
TacShell::~TacShell()
{
  if( mRenderer )
  {
    mRenderer->UnloadDefaultGraphicsObjects();
    delete mRenderer;
  }
  delete mLocalization;
  delete mFontStuff;
  delete mLog;
  delete mNet;
  delete mTimer;
  delete mInput;
}
void TacShell::Init( TacErrors& errors )
{
  // load settings
  {
    TacString settingsFilename = mAppName + "Settings.txt";
    auto settings = new TacSettings();
    settings->mPath = mPrefPath + "/" + settingsFilename;
    settings->Load( errors );
    TAC_HANDLE_ERROR( errors );
    mSettings = settings;
  }

  // create renderer
  {
    TacVector< TacRendererFactory* >& rendererFactories = TacRendererFactory::GetRegistry();
    if( rendererFactories.empty() )
    {
      errors = "No renderers available";
      return;
    }


    TacRendererFactory* rendererFactory = rendererFactories[ 0 ];
    TacString defaultRendererName = TacOS::Instance->GetDefaultRendererName();
    if( !defaultRendererName.empty() )
    {
      for( TacRendererFactory* curRendererFactory : rendererFactories )
      {
        if( curRendererFactory->mRendererName == defaultRendererName )
        {
          rendererFactory = curRendererFactory;
        }
      }
    }


    TacString rendererName = rendererFactory->mRendererName;
    rendererName = mSettings->GetString( nullptr,  { "DefaultRenderer" },rendererName,  errors );
    TAC_HANDLE_ERROR( errors );
    if( !TacFindIf( &rendererFactory, rendererFactories, [ & ]( TacRendererFactory* fact ) { return fact->mRendererName == rendererName; } ) )
      std::cout << "Failed to find " + rendererName + " renderer";
    TacRenderer* renderer = nullptr;
    rendererFactory->CreateRendererOuter( &renderer );
    renderer->mShell = this;
    renderer->Init( errors );
    mRenderer = renderer;
  }

  mJobQueue = new TacJobQueue;
  mJobQueue->Init();

  //mAssetManager = new TacAssetManager;
  mTextureAssetManager = new TacTextureAssetManager;
  //mTextureAssetManager->mAssetManager = mAssetManager;
  mTextureAssetManager->mJobQueue = mJobQueue;
  mTextureAssetManager->mRenderer = mRenderer;

  mFontStuff = new TacFontStuff();
  mFontStuff->Load( mSettings, mRenderer, TacFontAtlasDefaultVramByteCount, errors );
  TAC_HANDLE_ERROR( errors );

  mLocalization = new TacLocalization();
  mLocalization->Load( "assets/localization.txt", errors );
  TAC_HANDLE_ERROR( errors );

  mRenderer->LoadDefaultGraphicsObjects( errors );
  TAC_HANDLE_ERROR( errors );

  AddSoul( errors );
  TAC_HANDLE_ERROR( errors );

}
void TacShell::SetScopedGlobals()
{
  // The scope in which you refer to a global variable chooses which global to use
  // ( ie: exe global var or dll global var )
  ImGui::SetCurrentContext( mImGuiContext );
}
void TacShell::DebugImgui( TacErrors& errors )
{
  if( mLog )
    mLog->DebugImgui();
  if( mShowMainMenu && ImGui::BeginMainMenuBar() )
  {
    OnDestruct( ImGui::EndMainMenuBar() );
    if( ImGui::BeginMenu( "File" ) )
    {
      OnDestruct( ImGui::EndMenu() );

      ImGui::MenuItem( "Show ImGui Test Window", nullptr, &mImGuiShowTestWindow );
      ImGui::MenuItem( "Show Shell Window", nullptr, &mShowShellWindow );

      if( mLog )
        ImGui::MenuItem( "Log", nullptr, &mLog->mIsVisible );
      else if( ImGui::MenuItem( "Log" ) )
        mLog = new TacLog();
      ImGui::MenuItem( "(dummy menu)", NULL, false, false );
      if( ImGui::MenuItem( "New" ) ) {}
      if( ImGui::MenuItem( "Open", "Ctrl+O" ) ) {}
      if( ImGui::BeginMenu( "Open Recent" ) )
      {
        OnDestruct( ImGui::EndMenu() );
        ImGui::MenuItem( "fish_hat.c" );
        ImGui::MenuItem( "fish_hat.inl" );
        ImGui::MenuItem( "fish_hat.h" );
        if( ImGui::BeginMenu( "More.." ) )
        {
          OnDestruct( ImGui::EndMenu() );
          ImGui::MenuItem( "Sailor" );
        }
      }
    }
  }
  if( mImGuiShowTestWindow )
    ImGui::ShowTestWindow( &mImGuiShowTestWindow );
  for( TacSoul* soul : mSouls )
  {
    if( soul->mIsImGuiVisible )
      soul->DebugImgui( errors );
  }
  if( !mShowShellWindow )
    return;
  ImGui::Begin( "Shell", &mShowShellWindow );
  OnDestruct( ImGui::End() );
  TacVector< TacString > lines =
  {
    "//",
    "// TODO",
    "//",
    "// - Switch between Text shader and 2d shader",
    "// - Vertically center text",
    "//",
    "//",
    "//"
  };
  for( int i = 0; i < ( int )lines.size(); ++i )
  {
    const char* s = lines[ i ].c_str();
    float b = ( float )( i ) / lines.size();
    ImGui::TextColored( ImVec4( 1, 0, b, 1 ), s );
  }

  ImGui::Checkbox( "Pause", &mPaused );
  if( mPaused )
  {
    static int frameAdvanceCount = 1;
    ImGui::DragInt( "Frame advance count", &frameAdvanceCount );
    if( ImGui::Button( "Frame" ) )
    {
      for( int iFrame = 0; iFrame < frameAdvanceCount; ++iFrame )
      {
        Frame( errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
  }
  ImGui::Text( "Soul count: %i", mSouls.size() );
  ImGui::DragFloat( "Accumulated seconds", &mTimer->mAccumulatedSeconds );
  ImGui::DragInt( "mMouseRelTopLeftY", &mMouseRelTopLeftY );
  ImGui::DragInt( "mMouseRelTopLeftX", &mMouseRelTopLeftX );
  ImGui::DragInt( "mMouseRelTopLeftYDelta", &mMouseRelTopLeftYDelta );
  ImGui::DragInt( "mMouseRelTopLeftXDelta", &mMouseRelTopLeftXDelta );
  ImGui::DragInt( "mouse wheel rel", &mMouseWheelRel );
  ImGui::DragInt( "window w", &mWindowWidth );
  ImGui::DragInt( "window h", &mWindowHeight );
  ImGui::Checkbox( "imgui render", &mImGuiRender );
  ImGui::Text( "Elapsed time: %s", TacFormatFrameTime( mElapsedSeconds ).c_str() );



  mKeyboardInput->DebugImgui();

  if( mRenderer )
    mRenderer->DebugImgui();
  if( mNet )
    mNet->DebugImgui();
  if( mLocalization )
    mLocalization->DebugImgui();
  if( mFontStuff )
    mFontStuff->DebugImgui();
  if( mInput )
    mInput->DebugImgui();

  if( ImGui::Button( "std::cout" ) )
    std::cout << mToStdOut << std::endl;
  ImGui::SameLine();
  ImGui::InputText( "", mToStdOut );
  ImGui::Text( "Press " + ToString( TacToggleMainMenuKey ) + " to toggle main menu" );
  ImGui::Checkbox( "Show main menu", &mShowMainMenu );
  for( TacSoul* soul : mSouls )
    ImGui::Checkbox( soul->GetDebugName() + " visible", &soul->mIsImGuiVisible );
  if( mGhostCreateFn && ImGui::Button( "Create Ghost" ) )
  {
    AddSoul( errors );
    TAC_HANDLE_ERROR( errors );
  }
  mDebugImguiAux.EmitEvent();
}
void TacShell::AddSoul( TacErrors& errors )
{
  if( !mGhostCreateFn )
    return;
  TacSoul* ghost = mGhostCreateFn( this, errors );
  TAC_HANDLE_ERROR( errors );
  ghost->mID = mSoulIDCounter++;
  mSouls.push_back( ghost );
}


void TacShell::Update( TacErrors& errors )
{
  mTimer->Tick();
  if( mTimer->mAccumulatedSeconds < TAC_DELTA_FRAME_SECONDS )
    return;
  mTimer->mAccumulatedSeconds -= TAC_DELTA_FRAME_SECONDS;
  mOnRenderBegin.EmitEvent();
  //float greyf = 0.2f;
  //v4 greyv = { greyf, greyf, greyf, 1.0f };
  //TacTexture* backbufferColor = nullptr;
  //TacDepthBuffer* backbufferDepth = nullptr;
  if( mRenderer )
  {
    //mRenderer->GetBackbufferColor( &backbufferColor );
    //mRenderer->GetBackbufferDepth( &backbufferDepth );
    //mRenderer->ClearColor( backbufferColor, greyv );
    //mRenderer->ClearDepthStencil( backbufferDepth, true, 1.0f, false, 0 );
  }
  if( mNet )
  {
    mNet->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mOnUpdate.EmitEvent();
  if( mInput )
    mInput->Update();
  if( TacIsDebugMode() && mKeyboardInput->IsKeyJustDown( TacToggleMainMenuKey ) )
  {
    if( mImGuiRender )
    {
      mShowMainMenu = !mShowMainMenu;
    }
    else
    {
      mShowMainMenu = true;
      mImGuiRender = true;
    }
  }
  if( !mPaused )
  {
    Frame( errors );
    TAC_HANDLE_ERROR( errors );
  }
  if( mRenderer )
  {
    mRenderer->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
void TacShell::Frame( TacErrors& errors )
{
  for( auto soul : mSouls )
  {
    soul->Update( errors );
    if( errors.size() )
      return;
  }
  mElapsedSeconds += TAC_DELTA_FRAME_SECONDS;
}

