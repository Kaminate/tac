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

  void NumGridSys::DebugDraw3D( Errors& errors)
  {
    struct NumGridCBufData
    {
      u32 mWidth;
      u32 mHeight;
      u32 mBufferIndexOffset;
    };

    //static Vector<NumGridCBufData> sCPUCBufs;
    static Vector<Render::BufferHandle> sAllGPUNumbers;
    static Vector<u8> sAllCPUNumbers;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    sAllGPUNumbers.resize( Render::RenderApi::GetMaxGPUFrameCount() );

    Render::BufferHandle& allGPUNumbers{
      sAllGPUNumbers[ Render::RenderApi::GetCurrentRenderFrameIndex() ] };

    Render::IContext::Scope renderContextScope{ renderDevice->CreateRenderContext( errors) };
    Render::IContext* renderContext{ renderContextScope.GetContext() };

#if 0
    how to manage lifetime of gpu objects? (cbuf, data buf)

    theres the master editor world
    that gets copied to the game world

    that gets interpolated by netcode
    that gets interpolated by the frame rate

    the resulting World could be rendered into multiple viewports
    or there could be multiple worlds rendered.

    i could just Allocate, use, and delete immediately,
    relying on the deletion queue.

    as long as its the same World, it should have the same data buf.
    the cbuf would be associated with each World Render instance





#endif

    sAllCPUNumbers.clear();
    for( NumGrid* numGrid : mNumGrids )
    {
      if( numGrid->mData.empty() || !numGrid->mWidth || numGrid->mHeight )
        continue;

      TAC_ASSERT( numGrid->mData.size() == numGrid->mWidth * numGrid->mHeight );

      NumGridCBufData cbufData
      {
        .mWidth { numGrid->mWidth },
        .mHeight { numGrid->mHeight },
        .mBufferIndexOffset{ sAllCPUNumbers.size() },
      };

        

      numGrid->mData;


      numGrid->mData;
      numGrid->mWidth;
      numGrid->mHeight;

      //Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      //renderDevice->CreateRenderContext();
      //renderContext->SetVertexBuffer( {} );
      //renderContext->SetIndexBuffer( {} );


      Render::BufferHandle cbufhandle{
      renderDevice->CreateBuffer(
        Render::CreateBufferParams
        {
          .mByteCount     { sizeof(NumGridCBufData)},
          .mBytes         {},
          .mStride        {}, // used in creating the SRV and used for the input layout
          .mUsage         { Render::Usage::Default },
          .mBinding       { Render::Binding::None },
          .mCpuAccess     { Render::CPUAccess::None },
          .mGpuBufferMode { Render::GpuBufferMode::kUndefined },
          .mOptionalName  { "numgrid"},
          .mStackFrame    { TAC_STACK_FRAME },
        } );
      renderContext->;

      renderContext->UpdateBufferSimple( h, t, errors );
      renderContext->;

      renderContext->Draw(
        Render::DrawArgs
        {
          .mVertexCount { 6 * numGrid->mWidth * numGrid->mHeight },
        } );
    } );

    //Graphics* graphics{ Graphics::From( world ) };
    //graphics->

    TAC_CALL( renderContext->Execute( errors ) );
  }

  }

} // namespace Tac


