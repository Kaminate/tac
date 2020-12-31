
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
  struct Entity;
  struct Renderer;
  struct Shell;
  struct Soul;
  struct Texture;
  struct World;

  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    Vector< Entity* > mEntities;
    String            mDocumentPath;
  };

  struct PrefabCameraPosition
  {
    String mPrefab;
    Camera mCamera;
  };

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );
    Entity*             CreateEntity();
    bool                IsAnythingSelected();
    v3                  GetSelectionGizmoOrigin();
    void                ClearSelection();
    void                CheckDeleteSelected();
    void                UpdateSavedPrefabs();
    void                GetSavedPrefabs( Vector< String > & paths, Errors& );
    void                SavePrefabs();
    void                LoadPrefabs( Errors& );
    void                LoadPrefabAtPath( String , Errors& );
    void                LoadPrefabCameraPosition( Prefab* );
    void                SavePrefabCameraPosition( Prefab* );
    void                RemoveEntityFromPrefabRecursively( Entity* );
    Prefab*             FindPrefab( Entity* );
    void                ModifyPathRelative( String& path );
    void                DeleteSelectedEntities();
    void                CreatePropertyWindow( Errors& );
    void                CreateGameWindow( Errors& );
    void                CreateMainWindow( Errors& );
    void                CreateSystemWindow( Errors& );
    void                CreateProfileWindow( Errors& );
    DesktopWindowHandle CreateWindow( StringView );
    bool                ShouldCreateWindowNamed( StringView );
    void                GetWindowsJson( Json** outJson, Errors& );
    void                GetWindowsJsonData( StringView windowName, int* x, int* y, int* w, int* h );
    Json*               FindWindowJson( StringView windowName );
    String              mOnlyCreateWindowNamed;
    World*              mWorld = nullptr;
    Vector< Entity* >   mSelectedEntities;
    bool                mSelectedHitOffsetExists = false;
    v3                  mSelectedHitOffset = {};
    bool                mSelectedGizmo = false;
    v3                  mTranslationGizmoDir = {};
    float               mTranslationGizmoOffset = 0;
    Camera              mEditorCamera;
    Vector< Prefab* >   mPrefabs;
  };

  extern Creation gCreation;

  const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );

}

