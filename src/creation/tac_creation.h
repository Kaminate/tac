#pragma once

#include "src/common/containers/tac_vector.h"
#include "src/space/tac_space_types.h"

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

  struct SelectedEntities
  {
    bool                empty();
    int                 size();
    Entity**            begin();
    Entity**            end();
    v3                  GetGizmoOrigin();
    void                clear();
    void                DeleteEntitiesCheck();
    void                DeleteEntities();
    void                AddToSelection( Entity* );
    void                Select( Entity* );
    bool                IsSelected( Entity* );

    Vector< Entity* >   mSelectedEntities;
  };

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );


    //===-------------- Entities -------------===//
    RelativeSpace       GetEditorCameraVisibleRelativeSpace();
    Entity*             CreateEntity();
    Entity*             InstantiateAsCopy( Entity*, const RelativeSpace& );

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

    SelectedEntities    mSelectedEntities;
    String              mOnlyCreateWindowNamed;
    World*              mWorld = nullptr;
    bool                mSelectedHitOffsetExists = false;
    v3                  mSelectedHitOffset = {};
    bool                mSelectedGizmo = false;
    v3                  mTranslationGizmoDir = {};
    float               mTranslationGizmoOffset = 0;
    Camera*             mEditorCamera{};
    //GamePresentation*   mGamePresentation = nullptr;
    //SkyboxPresentation* mSkyboxPresentation = nullptr;

    bool                mUpdateAssetView = false;
    EntityUUIDCounter   mEntityUUIDCounter;
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  void                  ModifyPathRelative( String& path );

  extern Creation       gCreation;

} // namespace Tac

