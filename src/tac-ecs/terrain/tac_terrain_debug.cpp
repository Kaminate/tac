#include "tac-ecs/terrain/tac_terrain.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac
{
  struct TerainDebugger
  {
    void DebugImgui()
    {
      bool changed {};

      const FileSys::Path oldHeightmapPath { mTerrain->mHeightmapTexturePath };

      if( ImGuiCollapsingHeader( "Heightmap" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;

        ImGuiText( ShortFixedString::Concat( "Heightmap path: ", mTerrain->mHeightmapTexturePath ) );
        if( ImGuiButton( "Change Heightmap" ) )
        {
          mHeightmapFileDialogErrors.clear();
          mTerrain->mHeightmapTexturePath = AssetOpenDialog( mHeightmapFileDialogErrors );
        }

        if( !iterated || ImGuiButton("Refresh directory" ))
        {
          iterated = true;
          mHeightmapDirectoryIterateErrors.clear();
          heightmapPaths = FileSys::IterateFiles(
            FileSys::Path( "assets/heightmaps" ),
            FileSys::IterateType::Recursive, // OS::OSGetFilesInDirectoryFlags::Recursive,
            mHeightmapDirectoryIterateErrors );
        }

        for( const FileSys::Path& heightmapPath : heightmapPaths )
        {
          if( ImGuiButton( heightmapPath.u8string() ) )
          {
            Errors e;
            mTerrain->mHeightmapTexturePath = ModifyPathRelative( heightmapPath, e );
            TAC_ASSERT( !e );
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

      ImGuiText( String() + "Ground texture: " + mTerrain->mGroundTexturePath );
      ImGuiText( String() + "Noise texture: " + mTerrain->mNoiseTexturePath );

      if( ImGuiButton( "Open Ground Texture" ) )
      {
        mTerrain->mTestHeightmapLoadErrors.clear();
        mTerrain->mGroundTexturePath = AssetOpenDialog( mTerrainTextureDialogErrors );
      }

      if( ImGuiButton( "Open Noise Texture" ) )
      {
        mNoiseTextureDialogErrors.clear();
        mTerrain->mNoiseTexturePath = AssetOpenDialog( mNoiseTextureDialogErrors );
      }

      if( mTerrain->mTestHeightmapLoadErrors )
        ImGuiText( String() + "Load heightmap errors: " + mTerrain->mTestHeightmapLoadErrors.ToString() );

      if( mTerrainTextureDialogErrors )
        ImGuiText( String() + "Dialog heightmap errors: " + mTerrainTextureDialogErrors.ToString() );

      if( mNoiseTextureDialogErrors )
        ImGuiText( String() + "Dialog noise texture errors: " + mNoiseTextureDialogErrors.ToString() );

      if( mHeightmapFileDialogErrors )
        ImGuiText( String() + "Heightmap file dialog errors: " + mHeightmapFileDialogErrors.ToString() );

      if( mHeightmapDirectoryIterateErrors )
        ImGuiText( String() + "Heightmap directory iterate errors: " + mHeightmapDirectoryIterateErrors.ToString() );

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
      Entity* entity { mTerrain->mEntity };
      World* world { entity->mWorld };
      Debug3DDrawData* debug3DDrawData { world->mDebug3DDrawData };
      v3 gridColor  { 0, 0, 0 };
      v3 offset  { 0, 0.1f, 0 }; // for zfighting against the tri mesh

      if( mTerrain->mRowMajorGrid.empty() )
        return;

      if( !mDrawGrid )
        return;

      for( int iRow {}; iRow < mTerrain->mSideVertexCount; ++iRow )
      {
        for( int iCol {}; iCol < mTerrain->mSideVertexCount; ++iCol )
        {
          const v3 topLeft { mTerrain->GetGridVal( iRow, iCol ) + offset };

          if( iCol + 1 < mTerrain->mSideVertexCount )
          {
            const v3 topRight { mTerrain->GetGridVal( iRow, iCol + 1 ) + offset };
            debug3DDrawData->DebugDraw3DLine( topLeft, topRight, gridColor );
          }
          if( iRow + 1 < mTerrain->mSideVertexCount )
          {
            const v3 bottomLeft { mTerrain->GetGridVal( iRow + 1, iCol ) + offset };
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomLeft, gridColor );
          }
          if( iCol + 1 < mTerrain->mSideVertexCount && iRow + 1 < mTerrain->mSideVertexCount )
          {
            const v3 bottomRight { mTerrain->GetGridVal( iRow + 1, iCol + 1 ) + offset };
            debug3DDrawData->DebugDraw3DLine( topLeft, bottomRight, gridColor );
          }
        }
      }
    }

    bool mDrawGrid                          {};
    Errors mNoiseTextureDialogErrors        {};
    Errors mTerrainTextureDialogErrors      {};
    Errors mHeightmapFileDialogErrors       {};
    Errors mHeightmapDirectoryIterateErrors {};
    Terrain* mTerrain                       {};
    bool iterated                           {};
    Vector< FileSys::Path > heightmapPaths  {};
  } gTerrainDebugger;

  void TerrainDebugImgui( Terrain* terrain )
  {
    gTerrainDebugger.mTerrain = terrain;
    gTerrainDebugger.DebugImgui();
  }


}

