#include "tac_numgrid.h" // self-inc
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/containers/tac_vector_meta.h"
#include "tac-std-lib/containers/tac_array_meta.h"

namespace Tac
{
  TAC_META_REGISTER_CLASS_BEGIN( NumGrid );
  TAC_META_REGISTER_CLASS_MEMBER( mAsset );
  TAC_META_REGISTER_CLASS_MEMBER( mWidth );
  TAC_META_REGISTER_CLASS_MEMBER( mHeight );
  TAC_META_REGISTER_CLASS_MEMBER( mData );
  TAC_META_REGISTER_CLASS_MEMBER( mImages );
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
    const int oldW{ numGrid->mWidth };
    const int oldH{ numGrid->mHeight };
    TAC_ASSERT( numGrid->mData.size() == numGrid->mWidth * numGrid->mHeight );
    ImGuiText( "DebugNumGridComponent()" );
    ImGuiText( "Asset: " );
    ImGuiSameLine();
    ImGuiText( numGrid->mAsset.data() );
    ImGuiDragInt( "W", &numGrid->mWidth );
    ImGuiDragInt( "H", &numGrid->mHeight );
    numGrid->mWidth = Max( numGrid->mWidth, 1 );
    numGrid->mHeight = Max( numGrid->mHeight, 1 );
    if( oldW != numGrid->mWidth || oldH != numGrid->mHeight )
    {
      int newW{numGrid->mWidth};
      int newH{numGrid->mHeight};
      Vector< u8 > newData( newW * newH );
      for( int r{}; r < Min( oldH, newH ); ++r )
        for( int c{}; c < Min( oldW, newW ); ++c )
          newData[ r * newW + c ] = numGrid->mData[ r * oldW + c ];
      numGrid->mData.swap( newData );
    }

    static Errors numGridErrors;
    if( numGridErrors )
      ImGuiText( "Errors: %s", numGridErrors.ToString().c_str() );

    int firstFreeSpace{ -1 };
    for( int i{}; i < 256; ++i )
    {
      AssetPathStringView imgPath{ numGrid->mImages[ i ] };
      if( imgPath.empty() && firstFreeSpace == -1 )
        firstFreeSpace = i;

      if( imgPath.empty() )
        continue;

      Render::TextureHandle imgHandle{
             TextureAssetManager::GetTexture( imgPath, numGridErrors ) };
      if( !imgHandle.IsValid() )
        continue;
      v3i imgSize{ TextureAssetManager::GetTextureSize( imgPath, numGridErrors ) };
      float aspect{ imgSize.x / ( float )imgSize.y };

      ImGuiText( "Image %i", i );
      ImGuiSameLine();
      if( ImGuiButton( "Remove" ) )
        numGrid->mImages[ i ] = {};
      ImGuiSameLine();
      if( ImGuiButton( "Change" ) )
        if( AssetPathStringView newPath{ AssetOpenDialog( numGridErrors ) } )
          numGrid->mImages[ i ] = newPath;
      ImGuiSameLine();
      ImGuiImage( imgHandle.GetIndex(), v2( 100 * aspect, 100 ) );
    }

    if( firstFreeSpace != -1 )
    {
      if( ImGuiButton( "Add image" ) )
        if( AssetPathStringView newPath{ AssetOpenDialog( numGridErrors ) } )
          numGrid->mImages[ firstFreeSpace ] = newPath;
    }
      
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
  // ----------------------------------------------------
  // delete me begin
  // ----------------------------------------------------

  struct NumGrid2 : public Component
  {
    static auto GetComponent( Entity* ) -> NumGrid*;
    auto GetEntry() const -> const ComponentInfo* override;

    using GridImages = Array< AssetPathString, 256 >;

    AssetPathString mAsset  {};
    GridImages      mImages {};
    int             mWidth  {};
    int             mHeight {};
    Vector< u8 >    mData   {};
  };

    struct MetaNumGrid2 : public MetaCompositeType
    {
      using BaseType = NumGrid;

      MetaNumGrid2()
      {
        int offset = (uintptr_t)&(( ( NumGrid* )( nullptr ) )->mData);
        offset;

    Vector< u8 >    mData   {};

        MetaCompositeType::SetName( "NumGrid" );
        MetaCompositeType::SetSize( sizeof( NumGrid ) );
        void missing_semicolon();
          const MetaType*       metaType { &GetMetaType< decltype( BaseType::mImages ) >() };
        MetaCompositeType::AddMetaMember(

          MetaMember{
            .mName{ "mImages" },
            .mOffset{ ( ( int )( size_t ) & reinterpret_cast< char const volatile& >( ( ( ( BaseType* )0 )->mImages ) ) ) },
            .mMetaType{ metaType}, } );
        void missing_semicolon();
      }
    };
    static const MetaNumGrid sMetaNumGrid2;
    struct MetaType;
    const MetaType& GetMetaType( const NumGrid2& )
    {
      return sMetaNumGrid2;
    }; void missing_semicolon();

  // ----------------------------------------------------
  // delete me end
  // ----------------------------------------------------

  void NumGridSys::SpaceInitNumGrid()
  {

    MetaNumGrid2 foo;



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

  void NumGridSys::DebugDraw3D( const RenderParams& renderParams, Errors& errors )
  {
    struct NumGridCBufData
    {
      m4  mClipFromModel{};
      u32 mWidth{};
      u32 mHeight{};
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::IContext* renderContext{ renderParams.mContext };
    renderContext->DebugEventBegin( "NumGridSys::DebugEventBegin()" );
    renderContext->SetViewport( renderParams.mViewSize );
    renderContext->SetScissor( renderParams.mViewSize );
    renderContext->SetVertexBuffer( {} );
    renderContext->SetIndexBuffer( {} );
    renderContext->SetRenderTargets(
      Render::Targets
      {
        .mColors { renderParams.mColor },
        .mDepth  { renderParams.mDepth },
      } );

    static Render::ProgramHandle program{
      renderDevice->CreateProgram(
        Render::ProgramParams
        {
          .mInputs{ "NumGrid" },
          .mStackFrame{ TAC_STACK_FRAME },
        }, errors ) };
    static Render::PipelineHandle pipeline{
      renderDevice->CreatePipeline(
        Render::PipelineParams
        {
          .mProgram           { program },
          .mBlendState
          {
            .mSrcRGB   { Render::BlendConstants::One },
            .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
            .mBlendRGB { Render::BlendMode::Add },
            .mSrcA     { Render::BlendConstants::One },
            .mDstA     { Render::BlendConstants::One },
            .mBlendA   { Render::BlendMode::Add },
          },
          .mDepthState
          {
            .mDepthTest  { true },
            .mDepthWrite { false },
            .mDepthFunc  { Render::DepthFunc::Less },
          },
          .mRasterizerState
          {
            .mFillMode              { Render::FillMode::Solid },
            .mCullMode              { Render::CullMode::None },
            .mFrontCounterClockwise { true },
          },
          .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
          .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
          .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
          .mName              { "numgrid" },
          .mStackFrame        { TAC_STACK_FRAME },
        }, errors ) };

    static Render::BufferHandle cbufhandle{ renderDevice->CreateBuffer(
      Render::CreateBufferParams
      {
        .mUsage         { Render::Usage::Dynamic },
        .mBinding       { Render::Binding::ConstantBuffer },
        .mCpuAccess     { Render::CPUAccess::Write },
        .mOptionalName  { "numgrid_cbuf"},
        .mStackFrame    { TAC_STACK_FRAME },
      }, errors ) };
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->SetPipeline( pipeline );

    static Render::IShaderVar* cbufShaderVar{ renderDevice->GetShaderVariable( pipeline, "sNumGrid" ) };
    static Render::IShaderVar* texturesShaderVar{ renderDevice->GetShaderVariable( pipeline, "sTextures" ) };
    static Render::IShaderVar* textureIndicesShaderVar{ renderDevice->GetShaderVariable( pipeline, "sTextureIndices" ) };
    static Render::IShaderVar* samplerShaderVar{ renderDevice->GetShaderVariable( pipeline, "sSampler" ) };
    static Render::SamplerHandle sampler{
      renderDevice->CreateSampler(
        Render::CreateSamplerParams
        {
          .mFilter{ Render::Filter::Linear },
          .mName{ "numgrid_sampler" },
        } ) };

    Render::IBindlessArray::Binding unknownBinding{ TextureAssetManager::GetBindlessIndex( "assets/unknown.png", errors ) };
    Render::IBindlessArray* allTextures{ TextureAssetManager::GetBindlessArray() };



    cbufShaderVar->SetResource( cbufhandle );
    texturesShaderVar->SetBindlessArray( allTextures );
    samplerShaderVar->SetResource( sampler );

    const float aspect{ ( float )renderParams.mViewSize.x / ( float )renderParams.mViewSize.y };
    const m4 viewFromWorld{ renderParams.mCamera->View() };
    const m4 clipFromView{ renderParams.mCamera->Proj( aspect ) };
    const m4 clipFromWorld{ clipFromView * viewFromWorld };

    //static Entity sTempEntity;
    //static NumGrid snumGrid;
    //snumGrid.mWidth = 4;
    //snumGrid.mHeight = 2;
    //snumGrid.mData.resize( snumGrid.mWidth * snumGrid.mHeight );
    //snumGrid.mEntity = &sTempEntity;
    //NumGrid* tempGrids[] { &snumGrid };

    for( NumGrid* numGrid : mNumGrids )
    {
      TAC_ASSERT( numGrid->mData.size() == numGrid->mWidth * numGrid->mHeight );
      if( numGrid->mData.empty() )
        continue;

      Render::IBindlessArray::Binding texBindings[ 256 ] {};
      for( int i{}; i < 256; ++i )
      {
        texBindings[ i ] = TextureAssetManager::GetBindlessIndex( numGrid->mImages[ i ], errors );
        if( !texBindings[ i ].IsValid() )
          texBindings[i] = unknownBinding;
      }


      TAC_ASSERT( numGrid->mData.size() == numGrid->mWidth * numGrid->mHeight );

      const int cpuTextureIndicesByteCount{ ( int )( sizeof( u32 ) * numGrid->mWidth * numGrid->mHeight ) };
      dynmc u32* cpuTextureIndices = ( u32* )FrameMemoryAllocate( cpuTextureIndicesByteCount );
      for( int r{}; r < numGrid->mHeight; ++r )
      {
        for( int c{}; c < numGrid->mWidth; ++c )
        {
          int i{ numGrid->mWidth * r + c };
          u8 data{ numGrid->mData[ i ] };
          Render::IBindlessArray::Binding binding{ texBindings[ data ] };
          cpuTextureIndices[ i ] = (u32)binding.GetIndex();
        }
      }

      const Render::BufferHandle gridDataHandle{ renderDevice->CreateBuffer(
        Render::CreateBufferParams
        {
          .mByteCount     { cpuTextureIndicesByteCount },
          .mBytes         { cpuTextureIndices },
          .mStride        { sizeof( u32 ) },
          .mUsage         { Render::Usage::Default },
          .mBinding       { Render::Binding::ShaderResource },
          .mCpuAccess     { Render::CPUAccess::None },
          .mGpuBufferMode { Render::GpuBufferMode::kFormatted },
          .mGpuBufferFmt  { Render::TexFmt::kR32_uint },
          .mOptionalName  { "numgrid_data" },
          .mStackFrame    { TAC_STACK_FRAME },
        }, errors ) };

      const m4& worldFromModel{ numGrid->mEntity->mWorldTransform };
      const m4 clipFromModel{ clipFromWorld * worldFromModel };

      NumGridCBufData cbufData
      {
        .mClipFromModel { clipFromModel },
        .mWidth         { ( u32 )numGrid->mWidth },
        .mHeight        { ( u32 )numGrid->mHeight },
      };

      renderContext->UpdateBufferSimple( cbufhandle, cbufData, errors );
      textureIndicesShaderVar->SetResource( gridDataHandle );

      renderContext->CommitShaderVariables();
      renderContext->Draw(
        Render::DrawArgs
        {
          .mVertexCount { 6 * numGrid->mWidth * numGrid->mHeight }, // 6 = 2 * 3 tri verts
        } );

      renderDevice->DestroyBuffer( gridDataHandle );
    }

    renderContext->DebugEventEnd();
  }

} // namespace Tac


