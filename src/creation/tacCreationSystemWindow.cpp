#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/common/shell/tacShell.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/tacEntity.h"
#include "src/space/tacSystem.h"
#include "src/space/tacWorld.h"

namespace Tac
{
  static const SystemRegistryEntry* sSystemRegistryEntry;
  static const char* GetNSysPath()
  {
    return "SystemWindow.nSys";
  }

  CreationSystemWindow* CreationSystemWindow::Instance = nullptr;
  CreationSystemWindow::CreationSystemWindow()
  {
    Instance = this;
  }
  CreationSystemWindow::~CreationSystemWindow()
  {
    DesktopAppDestroyWindow( mDesktopWindowHandle );
    Instance = nullptr;
  }
  void CreationSystemWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    const String systemName = SettingsGetString( GetNSysPath(), "" );
    for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
      if( !StrCmp( entry.mName, systemName.c_str() ) )
        sSystemRegistryEntry = &entry;

    mDesktopWindowHandle = gCreation.CreateWindow( gSystemWindowName );
  };

  void CreationSystemWindow::ImGui()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "System Window" );

    if( ImGuiCollapsingHeader( "Select System" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( const SystemRegistryEntry& systemRegistryEntry : SystemRegistryIterator() )
      {
        if( ImGuiButton( systemRegistryEntry.mName ) )
        {
          sSystemRegistryEntry = &systemRegistryEntry;
          SettingsSetString( GetNSysPath(), systemRegistryEntry.mName );
        }
        if( sSystemRegistryEntry == &systemRegistryEntry )
        {
          ImGuiSameLine();
          ImGuiText( "<-- currently selected" );
        }
      }
    }

    if( sSystemRegistryEntry &&
        sSystemRegistryEntry->mDebugImGui &&
        ImGuiCollapsingHeader( FrameMemoryPrintf( "%s Debug", sSystemRegistryEntry->mName ) ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      System* system = gCreation.mWorld->GetSystem( sSystemRegistryEntry );
      sSystemRegistryEntry->mDebugImGui( system );
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
    TAC_PROFILE_BLOCK;
    DesktopAppResizeControls( mDesktopWindowHandle );
    DesktopAppMoveControls( mDesktopWindowHandle );
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    ImGui();
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );
    TAC_HANDLE_ERROR( errors );
  }


}

