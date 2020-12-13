#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacColorUtil.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTime.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreationPropertyWindow.h"
#include "src/creation/tacCreationMainWindow.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/creation/tacCreationProfileWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"
#include "src/space/model/tacModel.h"
#include "src/space/terrain/tacTerrain.h"
#include "src/space/tacSpace.h"
#include <iostream>
#include <functional>
#include <algorithm>

namespace Tac
{
	Creation gCreation;
	static void CreationInitCallback( Errors& errors ) { gCreation.Init( errors ); }
	static void CreationUninitCallback( Errors& errors ) { gCreation.Uninit( errors ); }
	static void CreationUpdateCallback( Errors& errors ) { gCreation.Update( errors ); }
	const char* prefabSettingsPath = "prefabs";
	const char* refFrameVecNames[] = {
		"mPos",
		"mForwards",
		"mRight",
		"mUp",
	};
	const char* axisNames[] = { "x", "y", "z" };

	void ExecutableStartupInfo::Init( Errors& errors )
	{
		TAC_UNUSED_PARAMETER( errors );
		mAppName = "Creation";
		mStudioName = "Sleeping Studio";
		mProjectInit = CreationInitCallback;
		mProjectUpdate = CreationUpdateCallback;
		mProjectUninit = CreationUninitCallback;
	}

	void Creation::Uninit( Errors& errors )
	{
		delete CreationMainWindow::Instance;
		delete CreationGameWindow::Instance;
		delete CreationPropertyWindow::Instance;
	}

	void Creation::CreatePropertyWindow( Errors& errors )
	{
		if( CreationPropertyWindow::Instance )
		{
			TAC_DELETE CreationPropertyWindow::Instance;
			return;
		}


		TAC_NEW CreationPropertyWindow;
		CreationPropertyWindow::Instance->Init( errors );
		TAC_HANDLE_ERROR( errors );
	}

	void Creation::CreateGameWindow( Errors& errors )
	{
		if( CreationGameWindow::Instance )
		{
			TAC_DELETE CreationGameWindow::Instance;
			return;
		}

		TAC_NEW CreationGameWindow;
		CreationGameWindow::Instance->Init( errors );
		TAC_HANDLE_ERROR( errors );
	}

	void Creation::CreateMainWindow( Errors& errors )
	{
		if( CreationMainWindow::Instance )
		{
			TAC_DELETE CreationMainWindow::Instance;
			return;
		}

		TAC_NEW CreationMainWindow;
		CreationMainWindow::Instance->Init( errors );
		TAC_HANDLE_ERROR( errors );
	}

	void Creation::CreateSystemWindow( Errors& errors )
	{
		if( CreationSystemWindow::Instance )
		{
			TAC_DELETE CreationSystemWindow::Instance;
			return;
		}

		TAC_NEW CreationSystemWindow;
		CreationSystemWindow::Instance->Init( errors );
		TAC_HANDLE_ERROR( errors );

	}

	void Creation::CreateProfileWindow( Errors& errors )
	{
		if( CreationProfileWindow::Instance )
		{
			TAC_DELETE CreationProfileWindow::Instance;
			return;
		}

		TAC_NEW CreationProfileWindow;
		CreationProfileWindow::Instance->Init( errors );
		TAC_HANDLE_ERROR( errors );

	}

	DesktopWindowHandle Creation::CreateWindow( StringView name )
	{
		int x, y, w, h;
		GetWindowsJsonData( name, &x, &y, &w, &h );
		return DesktopAppCreateWindow( x, y, w, h );
	}

	void Creation::GetWindowsJsonData( String windowName, int* x, int* y, int* w, int* h )
	{
		Json* windowJson = FindWindowJson( windowName );
		if( !windowJson )
		{
			*x = 200;
			*y = 200;
			*w = 400;
			*h = 300;
			return;
		}

		Errors errors;

		Vector< String > settingsPaths = { "Windows" };
		auto windowDefault = TAC_NEW Json;
		( *windowDefault )[ "Name" ] = gMainWindowName;

		*w = ( int )SettingsGetNumber( SettingsGetJson( "w", windowJson ), 400 );
		*h = ( int )SettingsGetNumber( SettingsGetJson( "h", windowJson ), 300 );
		*x = ( int )SettingsGetNumber( SettingsGetJson( "x", windowJson ), 200 );
		*y = ( int )SettingsGetNumber( SettingsGetJson( "y", windowJson ), 200 );
		const bool centered = ( int )SettingsGetBool( SettingsGetJson( "centered", windowJson ), false );
		TAC_HANDLE_ERROR( errors );

		if( centered )
		{
			CenterWindow( x, y, *w, *h );
		}
	}


	void Creation::GetWindowsJson( Json** outJson, Errors& errors )
	{
		Json* windows = SettingsGetJson( "Windows" );
		//if( windows->mElements.empty() )
		//{
		//}

		//*outJson = windows;
		//Vector< String > settingsPaths = { "Windows" };
		//auto windowDefault = TAC_NEW Json;
		//( *windowDefault )[ "Name" ] = gMainWindowName;

		//auto windowsDefault = TAC_NEW Json;
		//windowsDefault->mType = JsonType::Array;
		//windowsDefault->mElements.push_back( windowDefault );
		//Json* windows = settings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
		//TAC_HANDLE_ERROR( errors );

		*outJson = windows;
	}

	bool Creation::ShouldCreateWindowNamed( StringView name )
	{
		if( mOnlyCreateWindowNamed.size() && name != mOnlyCreateWindowNamed )
			return false;
		Json* windowJson = FindWindowJson( name );
		if( !windowJson )
			return false;

		Errors errors;
		const bool create = SettingsGetBool( SettingsGetJson( "Create", windowJson ), false );
		if( errors )
			return false;
		return create;
	}

	//void Creation::SetSavedWindowData( Json* windowJson, Errors& errors )
	//{

		//const StringView name = SettingsLookup({ "Name" }). GetString(  gMainWindowName, errors );
		//TAC_HANDLE_ERROR( errors );

		//const int width = ( int )SettingsLookup({ "w" }). GetNumber(  , 400, errors );
		//TAC_HANDLE_ERROR( errors );

		//const int height = ( int )SettingsLookup({ "h" }).GetNumber( windowJson, , 300, errors );
		//TAC_HANDLE_ERROR( errors );

		//int x = ( int )SettingsLookup({ "x" }).GetNumber( windowJson, , 200, errors );
		//TAC_HANDLE_ERROR( errors );

		//int y = ( int )SettingsLookup({ "y" }).GetNumber( windowJson, , 200, errors );
		//TAC_HANDLE_ERROR( errors );

		//const bool centered = ( int )SettingsLookup({ "centered" }).GetBool(  false);
		//TAC_HANDLE_ERROR( errors );

		//if( centered )
		//{
		//  CenterWindow( &x, &y, width, height );
		//}

		//WindowParams params;
		//params.mName = name;
		//params.mWidth = width;
		//params.mHeight = height;
		//params.mX = x;
		//params.mY = y;
		//DesktopWindowManager::Instance->SetWindowParams( params );
	//}

	//void Creation::SetSavedWindowsData( Errors& errors )
	//{
	//  Json* windows;
	//  GetWindowsJson( &windows, errors );
	//  TAC_HANDLE_ERROR( errors );
	//  for( Json* windowJson : windows->mElements )
	//  {
	//    SetSavedWindowData( windowJson, errors );
	//    TAC_HANDLE_ERROR( errors );
	//  }
	//}

	void Creation::Init( Errors& errors )
	{
		SpaceInit();
		mWorld = TAC_NEW World;
		mEditorCamera.mPos = { 0, 1, 5 };
		mEditorCamera.mForwards = { 0, 0, -1 };
		mEditorCamera.mRight = { 1, 0, 0 };
		mEditorCamera.mUp = { 0, 1, 0 };

		//ViewIdMainWindow = Render::CreateViewId();
		//ViewIdGameWindow = Render::CreateViewId();
		//ViewIdPropertyWindow = Render::CreateViewId();
		//ViewIdSystemWindow = Render::CreateViewId();
		//ViewIdProfileWindow = Render::CreateViewId();

		String dataPath;
		OS::GetApplicationDataPath( dataPath, errors );
		TAC_HANDLE_ERROR( errors );

		OS::CreateFolderIfNotExist( dataPath, errors );
		TAC_HANDLE_ERROR( errors );

		//SetSavedWindowsData( errors );
		TAC_HANDLE_ERROR( errors );

		Json* windows;
		GetWindowsJson( &windows, errors );
		TAC_HANDLE_ERROR( errors );

		mOnlyCreateWindowNamed = SettingsGetString( "onlyCreateWindowNamed", "" );
		TAC_HANDLE_ERROR( errors );

		// The first window spawned becomes the parent window
		if( ShouldCreateWindowNamed( gMainWindowName ) )
			CreateMainWindow( errors );
		TAC_HANDLE_ERROR( errors );

		if( ShouldCreateWindowNamed( gPropertyWindowName ) )
			CreatePropertyWindow( errors );
		TAC_HANDLE_ERROR( errors );

		if( ShouldCreateWindowNamed( gGameWindowName ) )
			CreateGameWindow( errors );
		TAC_HANDLE_ERROR( errors );

		if( ShouldCreateWindowNamed( gSystemWindowName ) )
			CreateSystemWindow( errors );
		TAC_HANDLE_ERROR( errors );

		if( ShouldCreateWindowNamed( gProfileWindowName ) )
			CreateProfileWindow( errors );
		TAC_HANDLE_ERROR( errors );


		LoadPrefabs( errors );
		TAC_HANDLE_ERROR( errors );
	}

	Json* Creation::FindWindowJson( StringView windowName )
	{
		Json* windows;
		Errors errors;
		GetWindowsJson( &windows, errors );
		if( errors )
			return nullptr;
		for( Json* windowJson : windows->mElements )
			if( windowJson->GetChild( "Name" ) == windowName )
				return windowJson;
		return nullptr;
	}

	void Creation::RemoveEntityFromPrefabRecursively( Entity* entity )
	{
		int prefabCount = mPrefabs.size();
		for( int iPrefab = 0; iPrefab < prefabCount; ++iPrefab )
		{
			Prefab* prefab = mPrefabs[ iPrefab ];
			bool removedEntityFromPrefab = false;
			int prefabEntityCount = prefab->mEntities.size();
			for( int iPrefabEntity = 0; iPrefabEntity < prefabEntityCount; ++iPrefabEntity )
			{
				if( prefab->mEntities[ iPrefabEntity ] == entity )
				{
					prefab->mEntities[ iPrefabEntity ] = prefab->mEntities[ prefabEntityCount - 1 ];
					prefab->mEntities.pop_back();
					if( prefab->mEntities.empty() )
					{
						mPrefabs[ iPrefab ] = mPrefabs[ prefabCount - 1 ];
						mPrefabs.pop_back();
					}

					removedEntityFromPrefab = true;
					break;
				}
			}

			if( removedEntityFromPrefab )
				break;
		}
		for( Entity* child : entity->mChildren )
			RemoveEntityFromPrefabRecursively( child );
	}

	void Creation::DeleteSelectedEntities()
	{
		Vector< Entity* > topLevelEntitiesToDelete;

		for( Entity* entity : mSelectedEntities )
		{
			bool isTopLevel = true;
			for( Entity* parent = entity->mParent; parent; parent = parent->mParent )
			{
				if( Contains( mSelectedEntities, parent ) )
				{
					isTopLevel = false;
					break;
				}
			}
			if( isTopLevel )
				topLevelEntitiesToDelete.push_back( entity );
		}
		for( Entity* entity : topLevelEntitiesToDelete )
		{
			RemoveEntityFromPrefabRecursively( entity );
			mWorld->KillEntity( entity );
		}
		mSelectedEntities.clear();
	}

	void Creation::Update( Errors& errors )
	{
		/*TAC_PROFILE_BLOCK*/;

    // dont need this
		//ImGuiSetNextWindowHandle( DesktopWindowHandle() );

		ImGuiBegin( "hello" );
		ImGuiEnd();


		if( !CreationMainWindow::Instance &&
				!CreationGameWindow::Instance &&
				!CreationPropertyWindow::Instance &&
				!CreationSystemWindow::Instance &&
				!CreationProfileWindow::Instance )
		{
			//CreateMainWindow( errors );
			//CreateGameWindow( errors );
			//CreatePropertyWindow( errors );
			//CreateSystemWindow( errors );
			TAC_HANDLE_ERROR( errors );
		}

		if( CreationMainWindow::Instance )
		{
			CreationMainWindow::Instance->Update( errors );
			TAC_HANDLE_ERROR( errors );
		}

		if( CreationGameWindow::Instance )
		{
			CreationGameWindow::Instance->Update( errors );
			TAC_HANDLE_ERROR( errors );
		}

		if( CreationPropertyWindow::Instance )
		{
			CreationPropertyWindow::Instance->Update( errors );
			TAC_HANDLE_ERROR( errors );
		}

		if( CreationSystemWindow::Instance )
		{
			CreationSystemWindow::Instance->Update( errors );
			TAC_HANDLE_ERROR( errors );
		}

		if( CreationProfileWindow::Instance )
		{
			CreationProfileWindow::Instance->Update( errors );
			TAC_HANDLE_ERROR( errors );
		}

		mWorld->Step( TAC_DELTA_FRAME_SECONDS );


		CheckDeleteSelected();

		if( gKeyboardInput.IsKeyJustDown( Key::S ) &&
				gKeyboardInput.IsKeyDown( Key::Modifier ) )
		{
			SavePrefabs();
			if( CreationGameWindow::Instance )
			{
				CreationGameWindow::Instance->mStatusMessage = "Saved prefabs!";
				CreationGameWindow::Instance->mStatusMessageEndTime = Shell::Instance.mElapsedSeconds + 5.0f;
			}
		}


	}

	Entity* Creation::CreateEntity()
	{
		World* world = mWorld;
		String desiredEntityName = "Entity";
		int parenNumber = 1;
		for( ;; )
		{
			Entity* entity = world->FindEntity( desiredEntityName );
			if( !entity )
				break;
			desiredEntityName = "Entity (" + ToString( parenNumber ) + ")";
			parenNumber++;
		}

		Entity* entity = world->SpawnEntity( NullEntityUUID );
		entity->mName = desiredEntityName;
		mSelectedEntities = { entity };
		return entity;
	}

	bool Creation::IsAnythingSelected()
	{
		return mSelectedEntities.size();
	}

	v3 Creation::GetSelectionGizmoOrigin()
	{
		TAC_ASSERT( IsAnythingSelected() );
		// do i really want average? or like center of bounding circle?
		v3 runningPosSum = {};
		int selectionCount = 0;
		for( auto entity : mSelectedEntities )
		{
			runningPosSum +=
				( entity->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
			entity->mRelativeSpace.mPosition;
			selectionCount++;
		}
		v3 averagePos = runningPosSum / ( float )selectionCount;
		v3 result = averagePos;
		if( mSelectedHitOffsetExists )
			result += mSelectedHitOffset;
		return result;
	}

	void Creation::ClearSelection()
	{
		mSelectedEntities.clear();
		mSelectedHitOffsetExists = false;
	}

	void Creation::CheckDeleteSelected()
	{
		CreationGameWindow* gameWindow = CreationGameWindow::Instance;

		if( !gameWindow || !gameWindow->mDesktopWindowHandle.IsValid() )
			return;

		DesktopWindowState* desktopWindowState = GetDesktopWindowState( gameWindow->mDesktopWindowHandle );

		if( !desktopWindowState->mNativeWindowHandle )
			return;

		if( !IsWindowHovered( gameWindow->mDesktopWindowHandle ) )
			return;

		if( !gKeyboardInput.IsKeyJustDown( Key::Delete ) )
			return;
		DeleteSelectedEntities();
	}

	void Creation::GetSavedPrefabs( Vector< String >& paths, Errors& errors )
	{
		TAC_UNUSED_PARAMETER( errors );
		Json* prefabs = SettingsGetJson( prefabSettingsPath );

		Vector< String > alreadySavedPrefabs;
		for( Json* child : prefabs->mElements )
			alreadySavedPrefabs.push_back( child->mString );
		paths = alreadySavedPrefabs;
	}

	void Creation::UpdateSavedPrefabs()
	{
		Json* prefabs = SettingsGetJson( prefabSettingsPath );

		Errors errors;
		Vector< String > alreadySavedPrefabs;
		GetSavedPrefabs( alreadySavedPrefabs, errors );
		TAC_HANDLE_ERROR( errors );

		for( Prefab* prefab : mPrefabs )
		{
			if( prefab->mDocumentPath.empty() )
				continue;
			if( Contains( alreadySavedPrefabs, prefab->mDocumentPath ) )
				continue;
			prefabs->AddChild()->SetString( prefab->mDocumentPath );
		}

		SettingsSave( errors );
	}

	Prefab* Creation::FindPrefab( Entity* entity )
	{
		for( Prefab* prefab : mPrefabs )
		{
			if( Contains( prefab->mEntities, entity ) )
			{
				return prefab;
			}
		}
		return nullptr;
	}

	void Creation::SavePrefabs()
	{
		for( Entity* entity : mWorld->mEntities )
		{
			if( entity->mParent )
				continue;

			Prefab* prefab = FindPrefab( entity );
			if( !prefab )
			{
				prefab = TAC_NEW Prefab;
				prefab->mEntities = { entity };
				mPrefabs.push_back( prefab );
			}


			// Get document paths for prefabs missing them
			if( prefab->mDocumentPath.empty() )
			{
				String savePath;
				String suggestedName =
					//prefab->mEntity->mName
					entity->mName +
					".prefab";
				Errors saveDialogErrors;
				OS::SaveDialog( savePath, suggestedName, saveDialogErrors );
				if( saveDialogErrors )
				{
					// todo: log it, user feedback
					std::cout << saveDialogErrors.ToString() << std::endl;
					continue;
				}

				ModifyPathRelative( savePath );

				prefab->mDocumentPath = savePath;
				UpdateSavedPrefabs();
			}

			//Entity* entity = prefab->mEntity;

			Json entityJson;
			entity->Save( entityJson );

			String prefabJsonString = entityJson.Stringify();
			Errors saveToFileErrors;
			void* bytes = prefabJsonString.data();
			int byteCount = prefabJsonString.size();
			OS::SaveToFile( prefab->mDocumentPath, bytes, byteCount, saveToFileErrors );
			if( saveToFileErrors )
			{
				// todo: log it, user feedback
				std::cout << saveToFileErrors.ToString() << std::endl;
				continue;
			}
		}
	}

	void Creation::ModifyPathRelative( String& savePath )
	{
		if( StartsWith( savePath, Shell::Instance.mInitialWorkingDir ) )
		{
			savePath = savePath.substr( Shell::Instance.mInitialWorkingDir.size() );
			savePath = StripLeadingSlashes( savePath );
		}
	}

	void Creation::LoadPrefabAtPath( String prefabPath, Errors& errors )
	{
		ModifyPathRelative( prefabPath );
		auto memory = TemporaryMemoryFromFile( prefabPath, errors );
		TAC_HANDLE_ERROR( errors );

		Json prefabJson;
		prefabJson.Parse( memory.data(), memory.size(), errors );

		Entity* entity = mWorld->SpawnEntity( NullEntityUUID );
		entity->Load( prefabJson );

		auto prefab = TAC_NEW Prefab;
		prefab->mDocumentPath = prefabPath;
		prefab->mEntities = { entity };
		mPrefabs.push_back( prefab );

		LoadPrefabCameraPosition( prefab );
		UpdateSavedPrefabs();
	}

	void Creation::LoadPrefabs( Errors& errors )
	{
		Vector< String > prefabPaths;
		GetSavedPrefabs( prefabPaths, errors );
		for( auto& prefabPath : prefabPaths )
		{
			LoadPrefabAtPath( prefabPath, errors );
			TAC_HANDLE_ERROR( errors );
		}
	}

	void Creation::LoadPrefabCameraPosition( Prefab* prefab )
	{
		if( prefab->mDocumentPath.empty() )
			return;
		v3* refFrameVecs[] = { &mEditorCamera.mPos,
													 &mEditorCamera.mForwards,
													 &mEditorCamera.mRight,
													 &mEditorCamera.mUp, };
		for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
		{
			v3* refFrameVec = refFrameVecs[ iRefFrameVec ];
			const StringView refFrameVecName = refFrameVecNames[ iRefFrameVec ];
			for( int iAxis = 0; iAxis < 3; ++iAxis )
			{
				const StringView axisName = axisNames[ iAxis ];
				Errors ignored;
				JsonNumber defaultValue = refFrameVecs[ iRefFrameVec ]->operator[]( iAxis );
				Json* json = &SettingsGetJson( "prefabCameraRefFrames" )->
					GetChild( prefab->mDocumentPath ).
					GetChild( refFrameVecName ).
					GetChild( axisName );
				JsonNumber axisValue = SettingsGetNumber( json, defaultValue );
				refFrameVec->operator[]( iAxis ) = ( float )axisValue;
			}
		}
	}

	void Creation::SavePrefabCameraPosition( Prefab* prefab )
	{
		if( prefab->mDocumentPath.empty() )
			return;
		Json* root = nullptr;

		v3 refFrameVecs[] = {
			mEditorCamera.mPos,
			mEditorCamera.mForwards,
			mEditorCamera.mRight,
			mEditorCamera.mUp,
		};
		for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
		{
			v3 refFrameVec = refFrameVecs[ iRefFrameVec ];
			String refFrameVecName = refFrameVecNames[ iRefFrameVec ];
			for( int iAxis = 0; iAxis < 3; ++iAxis )
			{
				const StringView axisName = axisNames[ iAxis ];
				Json* json = &SettingsGetJson( "prefabCameraRefFrames" )->
					GetChild( prefab->mDocumentPath ).
					GetChild( refFrameVecName ).
					GetChild( axisName );
				SettingsSetNumber( json, refFrameVec[ iAxis ] );
			}
		}
	}
}

