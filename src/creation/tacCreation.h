#pragma once

#include "src/common/containers/tacVector.h"

namespace Tac
{
  struct Errors;
  struct Camera;
  struct Json;
  struct Entity;
  struct World;
  struct StringView;
  struct DesktopWindowHandle;
  struct RelativeSpace;
  struct GamePresentation;
  struct SkyboxPresentation;

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );


    //===-------------- Entities -------------===//
    RelativeSpace       GetEditorCameraVisibleRelativeSpace();
    Entity*             CreateEntity();
    Entity*             InstantiateAsCopy( Entity*, const RelativeSpace& );


    //===-------------- Selection ------------===//
    bool                IsAnythingSelected();
    v3                  GetSelectionGizmoOrigin();
    void                ClearSelection();
    void                CheckDeleteSelected();
    void                DeleteSelectedEntities();



    //===-------------- Windows --------------===//
    void                CreateInitialWindow( const char*,
                                             // intellisense freaks out if i dont name fn
                                             void ( Creation:: * fn)( Errors& ),
                                             Errors& );
    void                CreateInitialWindows( Errors& );
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
    Camera*             mEditorCamera;
    GamePresentation*   mGamePresentation = nullptr;
    SkyboxPresentation* mSkyboxPresentation = nullptr;

    bool                mUpdateAssetView = false;
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  void                  ModifyPathRelative( String& path );

  extern Creation       gCreation;

}

