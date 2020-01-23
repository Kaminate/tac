#include "common/graphics/tacUI2D.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/graphics/imgui/tacImGui.h"
#include "common/tacOS.h"
#include "creation/tacCreation.h"
#include "creation/tacCreationSystemWindow.h"
#include "space/tacworld.h"
#include "space/tacentity.h"
#include "space/tacsystem.h"
#include "shell/tacDesktopApp.h"

const  TacSettingPath iSysPath = { "SystemWindow", "iSys" };

TacCreationSystemWindow::~TacCreationSystemWindow()
{
  delete mUI2DDrawData;
}
void TacCreationSystemWindow::Init( TacErrors& errors )
{
  TacShell* shell = mShell;
  mUI2DDrawData = new TacUI2DDrawData;
  mUI2DDrawData->mRenderView = mDesktopWindow->mRenderView;

  TacSettings* settings = shell->mSettings;
  mSystemIndex = ( int )settings->GetNumber( nullptr, iSysPath, -1, errors );
};
void TacCreationSystemWindow::ImGui()
{
  TacShell* shell = mShell;
  TacSystemRegistry* systemRegistry = TacSystemRegistry::Instance();

  SetCreationWindowImGuiGlobals( shell, mDesktopWindow, mUI2DDrawData );
  TacImGuiBegin( "System Window", {} );

  if( TacImGuiCollapsingHeader( "Select System" ) )
  {
    TAC_IMGUI_INDENT_BLOCK;
    for( int i = 0; i < systemRegistry->mEntries.size(); ++i )
    {
      TacSystemRegistryEntry* systemRegistryEntry = systemRegistry->mEntries[ i ];
      if( TacImGuiButton( systemRegistryEntry->mName ) )
      {
        mSystemIndex = i;
        TacErrors e;
        shell->mSettings->SetNumber( nullptr, iSysPath, mSystemIndex, e);
      }
      if( mSystemIndex == i )
      {
        TacImGuiSameLine();
        TacImGuiText( "<-- currently selected" );
      }
    }
  }

  if( mSystemIndex >= 0 && mSystemIndex < systemRegistry->mEntries.size() )
  {
    TacSystemRegistryEntry* systemRegistryEntry = systemRegistry->mEntries[ mSystemIndex ];
    TacImGuiText( systemRegistryEntry->mName );
    if( systemRegistryEntry->mDebugImGui )
    {
      TacSystem* system = mCreation->mWorld->GetSystem( systemRegistryEntry );
      systemRegistryEntry->mDebugImGui( system );
    }
  }

  // to force directx graphics specific window debugging
  if( TacImGuiButton( "close window" ) )
  {
    mDesktopWindow->mRequestDeletion = true;
  }
  TacImGuiEnd();
}
void TacCreationSystemWindow::Update( TacErrors& errors )
{
  TacShell* shell = mShell;
  mDesktopWindow->SetRenderViewDefaults();
  ImGui();
  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}

