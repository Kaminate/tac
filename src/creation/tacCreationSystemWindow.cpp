
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacShell.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacOS.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSystem.h"
#include "src/shell/tacDesktopApp.h"

namespace Tac
{
const  SettingPath iSysPath = { "SystemWindow", "iSys" };

CreationSystemWindow::~CreationSystemWindow()
{
  delete mUI2DDrawData;
}
void CreationSystemWindow::Init( Errors& errors )
{
  ;
  mUI2DDrawData = new UI2DDrawData;

  Settings* settings = Shell::Instance->mSettings;
  mSystemIndex = ( int )settings->GetNumber( nullptr, iSysPath, -1, errors );
};
void CreationSystemWindow::ImGui()
{
  ;
  SystemRegistry* systemRegistry = SystemRegistry::Instance();

  SetCreationWindowImGuiGlobals( mDesktopWindow,
                                 mUI2DDrawData,
                                 mDesktopWindowState.mWidth,
                                 mDesktopWindowState.mHeight );
  ImGuiBegin( "System Window", {} );

  if( ImGuiCollapsingHeader( "Select System" ) )
  {
    TAC_IMGUI_INDENT_BLOCK;
    for( int i = 0; i < systemRegistry->mEntries.size(); ++i )
    {
      SystemRegistryEntry* systemRegistryEntry = systemRegistry->mEntries[ i ];
      if( ImGuiButton( systemRegistryEntry->mName ) )
      {
        mSystemIndex = i;
        Errors e;
        Shell::Instance->mSettings->SetNumber( nullptr, iSysPath, mSystemIndex, e);
      }
      if( mSystemIndex == i )
      {
        ImGuiSameLine();
        ImGuiText( "<-- currently selected" );
      }
    }
  }

  if( mSystemIndex >= 0 && mSystemIndex < systemRegistry->mEntries.size() )
  {
    SystemRegistryEntry* systemRegistryEntry = systemRegistry->mEntries[ mSystemIndex ];
    ImGuiText( systemRegistryEntry->mName );
    if( systemRegistryEntry->mDebugImGui )
    {
      System* system = mCreation->mWorld->GetSystem( systemRegistryEntry );
      systemRegistryEntry->mDebugImGui( system );
    }
  }

  // to force directx graphics specific window debugging
  if( ImGuiButton( "close window" ) )
  {
    mDesktopWindow->mRequestDeletion = true;
  }
  ImGuiEnd();
}
void CreationSystemWindow::Update( Errors& errors )
{
  ;
  mDesktopWindow->SetRenderViewDefaults();
  ImGui();
  mUI2DDrawData->DrawToTexture( 0, 0, 0, errors );
  TAC_HANDLE_ERROR( errors );
}


}

