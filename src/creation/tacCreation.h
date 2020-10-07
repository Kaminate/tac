
#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/tacCamera.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacEvent.h"
#include "src/common/tacMemory.h"
#include "src/common/tacSettings.h"
#include "src/common/tacShell.h"

namespace Tac
{

  struct Creation;
  struct CreationMainWindow;
  struct CreationGameWindow;
  struct CreationPropertyWindow;
  struct CreationSystemWindow;
  struct CreationProfileWindow;
  struct DesktopApp;
  /*struct DesktopWindow*/;
  struct Entity;
  struct Renderer;
  struct Shell;
  struct Soul;
  struct Texture;
  struct UI2DDrawData;
  //struct WindowParams;
  struct World;


  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    Vector< Entity* > mEntities;
    String mDocumentPath;
    //String GetDisplayName();
  };

  struct PrefabCameraPosition
  {
    String mPrefab;
    Camera mCamera;
  };

  const Render::ViewId ViewIdMainWindow = 0;
  const Render::ViewId ViewIdGameWindow = 1;
  const Render::ViewId ViewIdPropertyWindow = 2;
  const Render::ViewId ViewIdSystemWindow = 3;
  const Render::ViewId ViewIdProfileWindow = 4;


  struct WindowFramebufferInfo
  {
    DesktopWindowHandle mDesktopWindowHandle;
    Render::FramebufferHandle mFramebufferHandle;
  };

  struct WindowFramebufferManager
  {
    static WindowFramebufferManager Instance;
    Vector< WindowFramebufferInfo > mWindowFramebufferInfos;
    WindowFramebufferInfo* FindWindowFramebufferInfo( DesktopWindowHandle );
    void Update( DesktopWindowStateCollection* oldStates,
                 DesktopWindowStateCollection* newStates  );
  };

  struct Creation // : public UpdateThing
  {
    static Creation* Instance;
    Creation();
    ~Creation();
    void Init( Errors& errors );// override;
    void SetSavedWindowData( Json* windowJson, Errors& errors );
    void SetSavedWindowsData( Errors& errors );
    void Update( Errors& errors );// override;
    Entity* CreateEntity();
    bool IsAnythingSelected();
    v3 GetSelectionGizmoOrigin();
    void ClearSelection();
    void CheckDeleteSelected();

    // Prefabs
    void UpdateSavedPrefabs();
    void GetSavedPrefabs( Vector< String > & paths, Errors& errors );
    void SavePrefabs();
    void LoadPrefabs( Errors& errors );
    void LoadPrefabAtPath( String path, Errors& errors );
    void LoadPrefabCameraPosition( Prefab* prefab );
    void SavePrefabCameraPosition( Prefab* prefab );
    void RemoveEntityFromPrefabRecursively( Entity* entity );
    Prefab* FindPrefab( Entity* entity );

    void ModifyPathRelative( String& path );
    void DeleteSelectedEntities();

    void CreatePropertyWindow( Errors& errors );
    void CreateGameWindow( Errors& errors );
    void CreateMainWindow( Errors& errors );
    void CreateSystemWindow( Errors& errors );
    void CreateProfileWindow( Errors& errors );
    DesktopWindowHandle CreateWindow( StringView );

    //void CreateDesktopWindow( String windowName, DesktopWindow** outDesktopWindow, Errors& errors );
    bool ShouldCreateWindowNamed( StringView name );
    void GetWindowsJson( Json** outJson, Errors& errors );
    void GetWindowsJsonData( String windowName, int* x, int* y, int* w, int* h );
    Json* FindWindowJson( StringView windowName );

    String mOnlyCreateWindowNamed;

    World* mWorld = nullptr;

    // todo: HashSet
    Vector< Entity* > mSelectedEntities;
    bool mSelectedHitOffsetExists = false;
    v3 mSelectedHitOffset = {};

    bool mSelectedGizmo = false;
    v3 mTranslationGizmoDir = {};
    float mTranslationGizmoOffset = 0;

    Camera mEditorCamera;

    // todo: HashSet
    Vector< Prefab* > mPrefabs;


    //DesktopWindowStates mDesktopWindowStates;

  };

  void SetCreationWindowImGuiGlobals( const DesktopWindowState* desktopWindowState,
                                      UI2DDrawData* ui2DDrawData );

  const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );

}

