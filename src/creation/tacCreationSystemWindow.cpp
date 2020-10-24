

#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacOS.h"
#include "src/common/tacShell.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSystem.h"
#include "src/space/tacWorld.h"

namespace Tac
{
  const SettingPath nSysPath = { "SystemWindow", "nSys" };

  CreationSystemWindow* CreationSystemWindow::Instance = nullptr;
  CreationSystemWindow::CreationSystemWindow()
  {
    Instance = this;
  }
  CreationSystemWindow::~CreationSystemWindow()
  {
    Instance = nullptr;
    delete mUI2DDrawData;
  }
  void CreationSystemWindow::Init( Errors& errors )
  {
    mUI2DDrawData = new UI2DDrawData;
    mSystemName = Settings::Instance->GetString( nullptr, nSysPath, "", errors );
    mDesktopWindowHandle = Creation::Instance->CreateWindow( gSystemWindowName );
  };
  void CreationSystemWindow::ImGui()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState(mDesktopWindowHandle);
    if( !desktopWindowState )
      return;

    ImGuiBegin( "System Window", {}, mDesktopWindowHandle );

    ImGuiText( "hello" );
    if( ImGuiCollapsingHeader( "Select System" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( const SystemRegistryEntry& systemRegistryEntry : *SystemRegistry::Instance() )
      {
        if( ImGuiButton( systemRegistryEntry.mName ) )
        {
          mSystemName = systemRegistryEntry.mName;
          Errors e;
          Settings::Instance->SetString( nullptr, nSysPath, mSystemName, e );
        }
        if( mSystemName == systemRegistryEntry.mName )
        {
          ImGuiSameLine();
          ImGuiText( "<-- currently selected" );
        }
      }
    }

    if( !mSystemName.empty() )
    {
      const SystemRegistryEntry* systemRegistryEntry = SystemRegistry::Instance()->Find( mSystemName );
      ImGuiText( systemRegistryEntry->mName );
      if( systemRegistryEntry->mDebugImGui )
      {
        System* system = Creation::Instance->mWorld->GetSystem( systemRegistryEntry );
        systemRegistryEntry->mDebugImGui( system );
      }
    }

    // to force directx graphics specific window debugging
    //if( ImGuiButton( "close window" ) )
    //{
    //  mDesktopWindow->mRequestDeletion = true;
    //}
    ImGuiEnd();
  }
  void CreationSystemWindow::Update( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState(mDesktopWindowHandle);
    if( !desktopWindowState )
      return;
    ImGui();
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, ScissorRect( w, h ) );
    mUI2DDrawData->DrawToTexture( viewHandle,
      desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }


}

