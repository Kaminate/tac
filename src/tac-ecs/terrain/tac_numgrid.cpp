#include "tac_numgrid.h" // self-inc
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/meta/tac_meta_composite.h"

namespace Tac
{
  TAC_META_REGISTER_CLASS_BEGIN( NumGrid );
  TAC_META_REGISTER_CLASS_MEMBER( mAsset );
  TAC_META_REGISTER_CLASS_END( NumGrid );

  static auto CreateNumGridSystem() -> System* { return TAC_NEW NumGridSys; }

  static void NumGridDebugImGui( System* )
  {
    ImGuiText( "NumGridDebugImGui()" );
  }

  static auto CreateNumGridComponent( World* world ) -> Component*
  {
    return NumGridSys::GetSystem( world )->CreateNumGrid();
  }

  static void DestroyNumGridComponent( World* world, Component* component )
  {
    NumGridSys::GetSystem( world )->DestroyNumGrid( ( NumGrid* )component );
  }

  static void DebugNumGridComponent( Component* component )
  {
    NumGrid* numGrid{ ( NumGrid* )component };
    ImGuiText( "DebugNumGridComponent()" );

    ImGuiText( "Asset: " );
    ImGuiSameLine();
    ImGuiText(numGrid->mAsset.data());
    ImGuiDragInt( "W", &numGrid->mWidth );
    ImGuiDragInt( "H", &numGrid->mHeight );
  }

  static ComponentInfo* sRegistry         {};
  SystemInfo*           NumGridSys::sInfo {};

  auto NumGrid::GetComponent( Entity* entity ) -> NumGrid*
  {
    return ( NumGrid* )entity->GetComponent( sRegistry );
  }
  auto NumGrid::GetEntry() const -> const ComponentInfo*
  {
    return sRegistry;
  }

  auto NumGridSys::CreateNumGrid() -> NumGrid*
  {
    auto numGrid { TAC_NEW NumGrid };
    mNumGrids.insert( numGrid );
    return numGrid;
  }

  void NumGridSys::DestroyNumGrid( NumGrid* numGrid )
  {
    mNumGrids.erase( numGrid );
    TAC_DELETE numGrid;
  }

  void NumGridSys::Update()
  {
  }

  void NumGridSys::DebugImgui()
  {
  }

  void NumGridSys::SpaceInitNumGrid()
  {
    sInfo = SystemInfo::Register();
    sInfo->mName = "NumGrid";
    sInfo->mCreateFn = CreateNumGridSystem;
    sInfo->mDebugImGui = NumGridDebugImGui;

    *( sRegistry = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName         { "NumGrid" },
      .mCreateFn     { CreateNumGridComponent },
      .mDestroyFn    { DestroyNumGridComponent },
      .mDebugImguiFn { DebugNumGridComponent },
      .mMetaType     { &GetMetaType< NumGrid >() }
    };
  }
  auto NumGridSys::GetSystem( dynmc World* world ) -> dynmc NumGridSys*
  {
    return ( dynmc NumGridSys* )world->GetSystem( NumGridSys::sInfo );
  }
  auto NumGridSys::GetSystem( const World* world ) -> const NumGridSys*
  {
    return ( const NumGridSys* )world->GetSystem( NumGridSys::sInfo );
  }


} // namespace Tac


