#pragma once

#include "src/common/tac_common.h"
#include "src/common/containers/tac_vector.h"
#include "src/space/tac_space_types.h"
#include "src/space/tac_space.h"

namespace Tac
{
  struct SelectedEntities
  {
    bool                empty() const;
    int                 size() const;
    Entity**            begin();
    Entity**            end();
    v3                  GetGizmoOrigin() const;
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
    DesktopWindowHandle CreateDesktopWindow( StringView );
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

    bool                mUpdateAssetView = false;
    EntityUUIDCounter   mEntityUUIDCounter;
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  //void                  ModifyPathRelative( Filesystem::Path& path );


  extern Creation       gCreation;

} // namespace Tac

