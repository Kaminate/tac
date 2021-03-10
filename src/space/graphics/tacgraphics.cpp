#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacMemory.h"
#include "src/common/tacPreprocessor.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/model/tacModel.h"
#include "src/space/skybox/tacSkyboxComponent.h"
#include "src/space/tacComponent.h"
#include "src/space/tacWorld.h"

#include <set>
#include <cmath>

namespace Tac
{

  struct GraphicsImpl : public Graphics
  {
    Model*              CreateModelComponent() override
    {
      auto model = TAC_NEW Model;
      mModels.insert( model );
      return model;
    }

    void                DestroyModelComponent( Model* model ) override
    {
      auto it = mModels.find( model );
      TAC_ASSERT( it != mModels.end() );
      mModels.erase( it );
      delete model;
    }
    Skybox*             CreateSkyboxComponent() override
    {
      auto skybox = TAC_NEW Skybox;
      mSkyboxes.insert( skybox );
      return skybox;
    }

    void                DestroySkyboxComponent( Skybox* skybox ) override
    {
      auto it = mSkyboxes.find( skybox );
      TAC_ASSERT( it != mSkyboxes.end() );
      mSkyboxes.erase( it );
      delete skybox;
    }

    void                VisitModels( ModelVisitor* modelVisitor ) override
    {
      for( Model* model : mModels )
        ( *modelVisitor )( model );
    }

    void                VisitSkyboxes( SkyboxVisitor* skyboxVisitor ) override
    {
      for( Skybox* skybox : mSkyboxes )
        ( *skyboxVisitor )( skybox );
    }

    std::set< Model* >  mModels;
    std::set< Skybox* > mSkyboxes;
  };

  static SystemRegistryEntry* gGraphicsSystemRegistryEntry;

  static System* CreateGraphicsSystem()
  {
    return TAC_NEW GraphicsImpl;
  }

  // Why does Graphics::DebugImgui() and GraphicsDebugImgui() exist
  void      Graphics::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Graphics" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );

    ImGuiText( __FUNCTION__ );
    ImGuiText( "Graphics::DebugImgui()" );
  }

  void      GraphicsDebugImgui( System* ); // Defined in tacgraphicsdebug.cpp

  void      RegisterGraphicsSystem()
  {
    gGraphicsSystemRegistryEntry = SystemRegisterNewEntry();
    gGraphicsSystemRegistryEntry->mCreateFn = CreateGraphicsSystem;
    gGraphicsSystemRegistryEntry->mName = "Graphics";
    gGraphicsSystemRegistryEntry->mDebugImGui = GraphicsDebugImgui;
    RegisterModelComponent();
    RegisterSkyboxComponent();
  }

  Graphics* GetGraphics( World* world )
  {
    return ( Graphics* )world->GetSystem( gGraphicsSystemRegistryEntry );
  }


}

