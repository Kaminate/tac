#include "src/space/terrain/tac_terrain.h"

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

namespace Tac
{
  struct TerainDebugger
  {
    void DebugImgui()
    {
      bool changed = false;

      const Filesystem::Path oldHeightmapPath = mTerrain->mHeightmapTexturePath;

      if( ImGuiCollapsingHeader( "Heightmap" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;


        ImGuiTextf( "Heightmap path: %s", mTerrain->mHeightmapTexturePath.u8string() );
        if( ImGuiButton( "Change Heightmap" ) )
        {
          mHeightmapFileDialogErrors.clear();
          Filesystem::Path path = OS::OSOpenDialog( mHeightmapFileDialogErrors );
          if( !path.empty() && mTerrain->mHeightmapTexturePath != path )
          {
            mTerrain->mHeightmapTexturePath = path;
          }
        }

        if( !iterated || ImGuiButton("Refresh directory" ))
        {
          iterated = true;
          mHeightmapDirectoryIterateErrors.clear();
          heightmapPaths =
            //OS::OSGetFilesInDirectory(
            Filesystem::IterateFiles(
              Filesystem::Path( "assets/heightmaps" ),
              Filesystem::IterateType::Recursive, // OS::OSGetFilesInDirectoryFlags::Recursive,
              mHeightmapDirectoryIterateErrors );
        }

        for( const Filesystem::Path& heightmapPath : heightmapPaths )
        {
          if( ImGuiButton( heightmapPath.u8string() ) )
          {
            mTerrain->mHeightmapTexturePath = heightmapPath;
          }
        }

        const bool changedHeightmap = mTerrain->mHeightmapTexturePath != oldHeightmapPath;
        if( changedHeightmap )
        {
          mTerrain->mTestHeightmapLoadErrors.clear();
          mTerrain->mTestHeightmapImageMemory.clear();
          mTerrain->LoadTestHeightmap();
          changed |= mTerrain->mTestHeightmapLoadErrors.empty();
        }
      }

      ImGuiCheckbox( "Draw grid", &mDrawGrid );

      ImGuiTextf( "Ground texture: %s", mTerrain->mGroundTexturePath.u8string().c_str() );
      ImGuiTextf( "Noise texture: %s", mTerrain->mNoiseTexturePath.u8string().c_str() );

      if( ImGuiButton( "Open Ground Texture" ) )
      {
        mTerrain->mTestHeightmapLoadErrors.clear();
        mTerrain->mGroundTexturePath = OS::OSOpenDialog(  mTerrainTextureDialogErrors );
      }

      if( ImGuiButton( "Open Noise Texture" ) )
      {
        mNoiseTextureDialogErrors.clear();
        mTerrain->mNoiseTexturePath = OS::OSOpenDialog(  mNoiseTextureDialogErrors );
      }

      if( mTerrain->mTestHeightmapLoadErrors )
        ImGuiTextf( "Load heightmap errors: %s", mTerrain->mTestHeightmapLoadErrors.ToString().c_str() );

      if( mTerrainTextureDialogErrors )
        ImGuiTextf( "Dialog heightmap errors: " , mTerrainTextureDialogErrors.ToString().c_str() );

      if( mNoiseTextureDialogErrors )
        ImGuiTextf( "Dialog noise texture errors: " , mNoiseTextureDialogErrors.ToString().c_str() );

      if( mHeightmapFileDialogErrors )
        ImGuiTextf( "Heightmap file dialog errors: %s", mHeightmapFileDialogErrors.ToString().c_str() );

      if( mHeightmapDirectoryIterateErrors )
        ImGuiTextf( "Heightmap directory iterate errors: %s", mHeightmapDirectoryIterateErrors.ToString().c_str() );

      changed |= ImGuiDragInt( "Subdivisions", &mTerrain->mSideVertexCount );
      changed |= ImGuiDragFloat( "Size", &mTerrain->mSideLength );
      changed |= ImGuiDragFloat( "Height", &mTerrain->mUpwardsHeight );
      changed |= ImGuiDragFloat( "Power", &mTerrain->mPower );
      if( changed )
      {
        mTerrain->Recompute();
      }

      if( mDrawGrid )
        DebugDraw();
    }

    void DebugDraw()
    {
      Entity* entity = mTerrain->mEntity;
      World* world = entity->mWorld;
      Debug3DDrawData* debug3DDrawData = world->mDebug3DDrawData;
      v3 gridColor = { 0, 0, 0 };
      v3 offset = { 0, 0.1f, 0 }; // for zfighting against the tri mesh

      if( mTerrain->mRowMajorGrid.empty() )
        return;

      if( !mDrawGrid )
        return;

      for( int iRow = 0; iRow < mTerrain->mSideVertexCount; ++iRow )
      {
        for( int iCol = 0; iCol < mTerrain->mSideVertexCount; ++iCol )
        {
          v3 topLeft = mTerrain->GetGridVal( iRow, iCol ) + offset;

          if( iCol + 1 < mTerrain->mSideVertexCount )
          {
            v3 topRight = mTerrain->GetGridVal( iRow, iCol + 1 ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, topRight, gridColor );
          }
          if( iRow + 1 < mTerrain->mSideVertexCount )
          {
            v3 bottomLeft = mTerrain->GetGridVal( iRow + 1, iCol ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomLeft, gridColor );
          }
          if( iCol + 1 < mTerrain->mSideVertexCount && iRow + 1 < mTerrain->mSideVertexCount )
          {
            v3 bottomRight = mTerrain->GetGridVal( iRow + 1, iCol + 1 ) + offset;
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomRight, gridColor );
          }
        }
      }
    }

    bool mDrawGrid = false;
    Errors mNoiseTextureDialogErrors;
    Errors mTerrainTextureDialogErrors;
    Errors mHeightmapFileDialogErrors;
    Errors mHeightmapDirectoryIterateErrors;
    Terrain* mTerrain = nullptr;
    bool iterated = false;
    Vector< Filesystem::Path > heightmapPaths;
  } gTerrainDebugger;

  void TerrainDebugImgui( Terrain* terrain )
  {
    gTerrainDebugger.mTerrain = terrain;
    gTerrainDebugger.DebugImgui();
  }


}

