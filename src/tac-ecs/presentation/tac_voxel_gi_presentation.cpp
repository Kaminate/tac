#include "tac_voxel_gi_presentation.h" // self-inc

#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/tac_ints.h"

#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/world/tac_world.h"

#if _MSC_VER
#pragma warning( disable: 4505 ) // ???
#endif

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()

namespace Tac
{
  // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
  // RWStructuredBuffer

  static Render::BlendStateHandle      voxelBlend;
  static Render::DepthStateHandle      voxelCopyDepthState;
  static Render::DepthStateHandle      voxelizeDepthState;
  static Render::FramebufferHandle     voxelFramebuffer;
  static Render::MagicBufferHandle     voxelRWStructuredBuf;
  static Render::RasterizerStateHandle voxelRasterizerState;
  static Render::ProgramHandle          voxelCopyShader;
  static Render::ProgramHandle          voxelVisualizerShader;
  static Render::ProgramHandle          voxelizerShader;
  static Render::TextureHandle         voxelFramebufferTexture;
  static Render::TextureHandle         voxelTextureRadianceBounce0;
  static Render::TextureHandle         voxelTextureRadianceBounce1;
  static Render::VertexDeclarations    voxelVertexDeclarations;
  static Render::VertexFormatHandle    voxelVertexFormat;
  static Render::ViewHandle            voxelView;

  struct VoxelSettings
  {
    int   voxelDimension = 1; // eventually 128
    bool  voxelDebug = true;
    bool  voxelDebugDrawVoxelOutlines = false;
    bool  voxelDebugDrawGridOutline = true;
    bool  voxelDebugDrawVoxels = true;
    bool  voxelEnabled = true;
    v3    voxelGridCenter = { 0, 0, 0 };
    float voxelGridHalfWidth = 10.0f;
    bool  voxelGridSnapCamera = false;
  };

  TAC_META_REGISTER_COMPOSITE_BEGIN( VoxelSettings )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelDimension )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelDebug )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelDebugDrawVoxelOutlines )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelDebugDrawGridOutline )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelDebugDrawVoxels )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelEnabled )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelGridCenter )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelGridHalfWidth )
    TAC_META_REGISTER_COMPOSITE_MEMBER( VoxelSettings, voxelGridSnapCamera )
    TAC_META_REGISTER_COMPOSITE_END( VoxelSettings );

  static VoxelSettings                 voxelSettingsCurrent;
  static VoxelSettings                 voxelSettingsSaved;

  struct CBufferVoxelizer
  {
    //       Position of the voxel grid in worldspace.
    //       It's not rotated, aligned to world axis.
    v3       gVoxelGridCenter;

    //       Half width of the entire voxel grid in worldspace
    float    gVoxelGridHalfWidth;

    //       Width of a single voxel
    float    gVoxelWidth;

    //       Number of voxels on each side of the grid
    u32 gVoxelGridSize;

    static Render::BufferHandle  Handle;

    static void Init();
  };

  Render::BufferHandle  CBufferVoxelizer::Handle;



  static Json*                   VoxelSettingsRoot() { return SettingsGetJson( "voxelgi" ); }



  static void                    CreateVoxelizeDepthState()
  {
    voxelizeDepthState = Render::CreateDepthState( { .mDepthTest = false,
                                                     .mDepthWrite = false,
                                                     .mDepthFunc = ( Render::DepthFunc )0 },
                                                    TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelCopyDepthState, "voxelize-depth" );
  }

  static void                    CreateVoxelDepthState()
  {
    voxelCopyDepthState = Render::CreateDepthState( { .mDepthTest = false,
                                                      .mDepthWrite = false,
                                                      .mDepthFunc = ( Render::DepthFunc )0 },
          TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelCopyDepthState, "vox-copy-depth" );
  }

  static void                    CreateVoxelView()
  {
    // The framebuffer texture is only to allow for renderdoc debugging pixel shaders
    const Render::CreateTextureParams CreateTextureParams
    {
      .mImage
      {
        .mWidth = voxelSettingsCurrent.voxelDimension,
        .mHeight = voxelSettingsCurrent.voxelDimension,
        .mFormat
        {
          .mElementCount = 4,
          .mPerElementByteCount = 1,
          .mPerElementDataType = Render::GraphicsType::unorm
        },
      },
      .mBinding = Render::Binding::RenderTarget
    };

    voxelFramebufferTexture = Render::CreateTexture( CreateTextureParams, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelFramebufferTexture, "voxel-fbo-tex" );

    voxelFramebuffer = Render::CreateFramebufferForRenderToTexture( { voxelFramebufferTexture }, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelFramebuffer, "voxel-fbo" );

    voxelView = Render::CreateView();
  }

  static void                    DestroyVoxelView()
  {
    Render::DestroyTexture( voxelFramebufferTexture, TAC_STACK_FRAME );
    Render::DestroyFramebuffer( voxelFramebuffer, TAC_STACK_FRAME );
    Render::DestroyView( voxelView );
  }

  void                    CBufferVoxelizer::Init()
  {
    CBufferVoxelizer::Handle = Render::CreateBuffer( "CBufferVoxelizer",
                                                             sizeof( CBufferVoxelizer ),
                                                             TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( CBufferVoxelizer::Handle, "vox-constant-buf" );
  }

  static void                    CreateVoxelRasterizerState()
  {
    voxelRasterizerState = Render::CreateRasterizerState( { .mFillMode = Render::FillMode::Solid,
                                                            .mCullMode = Render::CullMode::None,
                                                            .mFrontCounterClockwise = true,
                                                            .mScissor = false,
                                                            .mMultisample = false,
                                                            .mConservativeRasterization = true },
                                                          TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelRasterizerState, "voxel-rasterizer-state" );
  }

  static void                    CreateVoxelVisualizerShader(Errors& errors)
  {
    // This shader is used to debug visualize the voxel radiance
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::ProgramParams programParams
    {
      .mFileStem = "VoxelVisualizer",
      .mStackFrame = TAC_STACK_FRAME,
    };
    voxelVisualizerShader =renderDevice->CreateProgram( programParams, errors );
  }

  static void                    CreateVoxelCopyShader(Errors& errors)
  {
    // This shader is used to copy from the 3d magic buffer to the 3d texture
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::ProgramParams programParams
    {
      .mFileStem = "VoxelCopy",
      .mStackFrame = TAC_STACK_FRAME,
    };
    
    voxelCopyShader = renderDevice->CreateProgram( programParams, errors );
  }


  static void                    CreateVoxelizerBlend()
  {
    voxelBlend = Render::CreateBlendState( {}, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelBlend, "voxel-blend" );
  }

  static void                    CreateVoxelizerShader()
  {
    // The voxelizer shader turns geometry into a rwstructuredbuffer using atomics
    // to prevent flickering
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::ProgramParams programParams
    {
      .mFileStem = "",
      .mStackFrame = TAC_STACK_FRAME,
    };
    renderDevice->CreateProgram( programParams, errors );
    voxelizerShader = Render::CreateShader(  "Voxelizer" , TAC_STACK_FRAME );
  }

  static void                    CreateVertexFormat()
  {
    struct VoxelVtx
    {
      v3 pos;
      v3 nor;
      v2 uv;
    };

    const Render::VertexDeclaration posDecl
    {
      .mAttribute = Render::Attribute::Position,
      .mTextureFormat{.mElementCount = 3,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real },
      .mAlignedByteOffset = ( int )TAC_OFFSET_OF( VoxelVtx, pos )
    };
    const Render::VertexDeclaration norDecl
    {
      .mAttribute = Render::Attribute::Normal,
      .mTextureFormat{.mElementCount = 3,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real,},
      .mAlignedByteOffset = ( int )TAC_OFFSET_OF( VoxelVtx, nor ),
    };
    const Render::VertexDeclaration uvDecl
    {
      .mAttribute = Render::Attribute::Texcoord,
      .mTextureFormat{.mElementCount = 2,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real,},
      .mAlignedByteOffset = ( int )TAC_OFFSET_OF( VoxelVtx, uv ),
    };

    voxelVertexDeclarations.clear();
    voxelVertexDeclarations.push_back( posDecl );
    voxelVertexDeclarations.push_back( norDecl );
    voxelVertexDeclarations.push_back( uvDecl );

    voxelVertexFormat = Render::CreateVertexFormat( voxelVertexDeclarations,
                                                    voxelizerShader,
                                                    TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelVertexFormat, "vox-vtx-fmt" );
  }

  static Render::CreateTextureParams         GetVoxRadianceCreateTextureParams()
  {
    // rgba16f, 2 bytes (16 bits) per float, hdr values
    return Render::CreateTextureParams
    {
      .mImage =
      {
        .mWidth = voxelSettingsCurrent.voxelDimension,
        .mHeight = voxelSettingsCurrent.voxelDimension,
        .mDepth = voxelSettingsCurrent.voxelDimension,
        .mFormat =
        {
          .mElementCount = 4,
          .mPerElementByteCount = 2,
          .mPerElementDataType = Render::GraphicsType::real,
        },
      },
      .mPitch = 0,
      .mBinding = Render::Binding::ShaderResource | Render::Binding::UnorderedAccess,
      .mAccess = Render::Access::Default, 
      .mCpuAccess = Render::CPUAccess::None
    };
  }

  static void                    CreateVoxelTextureRadianceBounce1()
  {
    voxelTextureRadianceBounce1 = Render::CreateTexture( GetVoxRadianceCreateTextureParams(), TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelTextureRadianceBounce1, "radiance-bounce-1" );
  }

  static void                    CreateVoxelTextureRadianceBounce0()
  {
    voxelTextureRadianceBounce0 = Render::CreateTexture( GetVoxRadianceCreateTextureParams(), TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelTextureRadianceBounce0, "radiance-bounce-0" );
  }

  static void                    CreateVoxelRWStructredBuf()
  {
    const int voxelStride = sizeof( u32 ) * 2;
    const int voxelCount
      = voxelSettingsCurrent.voxelDimension
      * voxelSettingsCurrent.voxelDimension
      * voxelSettingsCurrent.voxelDimension;
    const Render::Binding binding = ( Render::Binding )(
      ( int )Render::Binding::ShaderResource |
      ( int )Render::Binding::UnorderedAccess );
    voxelRWStructuredBuf = Render::CreateMagicBuffer( voxelStride * voxelCount,
                                                      nullptr,
                                                      voxelStride,
                                                      binding,
                                                      Render::Access::Dynamic,
                                                      TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelRWStructuredBuf, "vox-rw-structured" );
  }

  static CBufferVoxelizer        VoxelGetCBuffer()
  {
    const float gVoxelWidth = ( voxelSettingsCurrent.voxelGridHalfWidth * 2.0f )
                            / voxelSettingsCurrent.voxelDimension;
    return { .gVoxelGridCenter    = voxelSettingsCurrent.voxelGridCenter,
             .gVoxelGridHalfWidth = voxelSettingsCurrent.voxelGridHalfWidth,
             .gVoxelWidth         = gVoxelWidth,
             .gVoxelGridSize      = (u32)voxelSettingsCurrent.voxelDimension };
  }

  static void                    RenderDebugVoxelOutlineGrid( Debug3DDrawData* drawData )
  {
    if( !voxelSettingsCurrent.voxelDebugDrawGridOutline )
      return;
    const v3 mini = voxelSettingsCurrent.voxelGridCenter - voxelSettingsCurrent.voxelGridHalfWidth * v3( 1, 1, 1 );
    const v3 maxi = voxelSettingsCurrent.voxelGridCenter + voxelSettingsCurrent.voxelGridHalfWidth * v3( 1, 1, 1 );
    drawData->DebugDraw3DAABB( mini, maxi );
  }

  static void                    RenderDebugVoxelOutlineVoxels( Debug3DDrawData* drawData )
  {
    if( !voxelSettingsCurrent.voxelDebugDrawVoxelOutlines )
      return;
    TAC_PROFILE_BLOCK;
    for( int i = 0; i < voxelSettingsCurrent.voxelDimension; ++i )
    {
      for( int j = 0; j < voxelSettingsCurrent.voxelDimension; ++j )
      {
        for( int k = 0; k < voxelSettingsCurrent.voxelDimension; ++k )
        {
          const float voxelWidth
            = ( voxelSettingsCurrent.voxelGridHalfWidth * 2.0f )
            / voxelSettingsCurrent.voxelDimension;
          const v3 iVoxel( ( float )i, ( float )j, ( float )k );
          const v3 minPos
            = voxelSettingsCurrent.voxelGridCenter
            - voxelSettingsCurrent.voxelGridHalfWidth * v3( 1, 1, 1 )
            + iVoxel * voxelWidth;
          const v3 maxPos = minPos + voxelWidth * v3( 1, 1, 1 );
          const v3 minColor = iVoxel / ( float )voxelSettingsCurrent.voxelDimension;
          const v3 maxColor = ( iVoxel + v3( 1, 1, 1 ) ) / ( float )voxelSettingsCurrent.voxelDimension;
          drawData->DebugDraw3DAABB( minPos, maxPos, minColor, maxColor );
        }
      }
    }
  }

  static void                    RenderDebugVoxelOutline( Debug3DDrawData* drawData )
  {
    RenderDebugVoxelOutlineGrid( drawData );
    RenderDebugVoxelOutlineVoxels( drawData );
  }

  static void                    RenderDebugVoxels( const Render::ViewHandle viewHandle )
  {
    TAC_PROFILE_BLOCK;
    if( !voxelSettingsCurrent.voxelDebugDrawVoxels )
      return;
    const CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();

    Render::BeginGroup( "Voxel GI Debug", TAC_STACK_FRAME );
    Render::UpdateConstantBuffer( CBufferVoxelizer::Handle,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );
    Render::SetShader( voxelVisualizerShader );
    Render::SetDepthState( GamePresentationGetDepthState() );
    Render::SetTexture( { voxelTextureRadianceBounce0 } );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetVertexBuffer( Render::BufferHandle(),
                             0,
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension );
    Render::SetIndexBuffer( Render::BufferHandle(), 0, 0 );
    Render::Submit( viewHandle, TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }

  static void                    VoxelGIPresentationRenderDebug( const World* world,
                                                                 const Camera* camera,
                                                                 const int viewWidth,
                                                                 const int viewHeight,
                                                                 const Render::ViewHandle viewHandle )
  {
    TAC_PROFILE_BLOCK;
    if( !voxelSettingsCurrent.voxelDebug )
      return;
    RenderDebugVoxels( viewHandle );
    RenderDebugVoxelOutline( world->mDebug3DDrawData );
  }

  static void                    VoxelGIPresentationRenderVoxelize( const World* world,
                                                                    const Camera* camera,
                                                                    const int viewWidth,
                                                                    const int viewHeight,
                                                                    const Render::ViewHandle viewHandle )
  {
    TAC_PROFILE_BLOCK;
    TAC_RENDER_GROUP_BLOCK( "Voxelize" );

    Render::DrawCallTextures textures = { Get1x1White() };

    Render::CBufferLights cBufferLights;
    struct : public LightVisitor
    {
      void operator()( Light* light ) override
      {
        if( cBufferLights->TryAddLight( LightToShaderLight( light ) ) )
          textures->push_back( light->mShadowMapDepth );
      }
      Render::CBufferLights* cBufferLights{};
      Render::DrawCallTextures* textures{};
    } lightVisitor;

    lightVisitor.cBufferLights = &cBufferLights;
    lightVisitor.textures = &textures;

    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             voxelVertexDeclarations,
                                                             errors );
        if( !mesh )
          return;

        const Render::DefaultCBufferPerObject objBuf =
        {
          .World = model->mEntity->mWorldTransform,
          .Color = Render::PremultipliedAlpha::From_sRGB( model->mColorRGB ),
        };

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          Render::BeginGroup( subMesh.mName, TAC_STACK_FRAME );
          Render::SetShader( voxelizerShader );
          Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
          Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
          Render::SetBlendState( voxelBlend );
          Render::SetRasterizerState( voxelRasterizerState );
          Render::SetSamplerState( { mSamplerState } );
          Render::SetDepthState( voxelizeDepthState );
          Render::SetVertexFormat( voxelVertexFormat );
          Render::SetTexture( *textures );
          Render::UpdateConstantBuffer( Render::DefaultCBufferPerObject::Handle,
                                        &objBuf,
                                        sizeof( Render::DefaultCBufferPerObject ),
                                        TAC_STACK_FRAME );
          Render::UpdateConstantBuffer( Render::CBufferLights::Handle,
                                        cBufferLights,
                                        sizeof( Render::CBufferLights ),
                                        TAC_STACK_FRAME );
          Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
          Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
          Render::Submit( voxelView, TAC_STACK_FRAME );
          Render::EndGroup( TAC_STACK_FRAME );
        }
      }

      Render::ViewHandle            mViewHandle;
      Render::SamplerStateHandle    mSamplerState;
      Render::DrawCallTextures* textures = nullptr;
      Render::CBufferLights* cBufferLights = nullptr;
    } modelVisitor;
    modelVisitor.mViewHandle = viewHandle;
    modelVisitor.mSamplerState = GamePresentationGetSamplerState();
    modelVisitor.textures = &textures;
    modelVisitor.cBufferLights = &cBufferLights;

    const CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();
    Render::UpdateConstantBuffer( CBufferVoxelizer::Handle,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );

    const Graphics* graphics = GetGraphics( world );
    graphics->VisitLights( &lightVisitor );
    graphics->VisitModels( &modelVisitor );
  }

  static void                    VoxelGIPresentationRenderVoxelCopy( const World* world,
                                                                     const Camera* camera,
                                                                     const int viewWidth,
                                                                     const int viewHeight,
                                                                     const Render::ViewHandle viewHandle )
  {
    TAC_PROFILE_BLOCK;
    Render::BeginGroup( "Voxel copy", TAC_STACK_FRAME );
    Render::SetShader( voxelCopyShader );
    Render::SetVertexBuffer( Render::BufferHandle(), 0,
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension );
    Render::SetIndexBuffer( Render::BufferHandle(), 0, 0 );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetDepthState( voxelCopyDepthState );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
    Render::SetPixelShaderUnorderedAccessView( voxelTextureRadianceBounce0, 1 );
    Render::Submit( Render::ViewHandle(), TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }

  static void                    VoxelSettingsUpdateSerialize()
  {
    const bool voxelSettingsChanged = 0 != MemCmp( &voxelSettingsCurrent,
                                                   &voxelSettingsSaved,
                                                   sizeof( VoxelSettings ) );
    if( !voxelSettingsChanged )
      return;

    GetMetaType< VoxelSettings >().JsonSerialize( VoxelSettingsRoot(), &voxelSettingsCurrent );
    SettingsSave();
    voxelSettingsSaved = voxelSettingsCurrent;
  }

  void VoxelGIPresentationInit( Errors& )
  {
    GetMetaType< VoxelSettings >().JsonDeserialize( VoxelSettingsRoot(), &voxelSettingsSaved );
    voxelSettingsCurrent = voxelSettingsSaved;
    CBufferVoxelizer::Init();
    CreateVoxelizerBlend();
    CreateVoxelizerShader();
    CreateVoxelView();
    CreateVoxelRasterizerState();
    CreateVoxelVisualizerShader();
    CreateVoxelCopyShader();
    CreateVoxelRWStructredBuf();
    CreateVoxelTextureRadianceBounce0();
    CreateVoxelTextureRadianceBounce1();
    CreateVertexFormat();
    CreateVoxelDepthState();
    CreateVoxelizeDepthState();
  }

  void VoxelGIPresentationUninit()
  {
    DestroyVoxelView();
  }

  void VoxelGIPresentationRender( const World* world,
                                  const Camera* camera,
                                  const int viewWidth,
                                  const int viewHeight,
                                  const Render::ViewHandle viewHandle )
  {
    if( !voxelSettingsCurrent.voxelEnabled )
      return;
    voxelSettingsCurrent.voxelGridCenter
      = voxelSettingsCurrent.voxelGridSnapCamera
      ? camera->mPos
      : voxelSettingsCurrent.voxelGridCenter;
    Render::SetViewFramebuffer( voxelView, voxelFramebuffer );
    Render::SetViewport( voxelView, Render::Viewport( voxelSettingsCurrent.voxelDimension,
                                                      voxelSettingsCurrent.voxelDimension ) );
    Render::SetViewScissorRect( voxelView, Render::ScissorRect( voxelSettingsCurrent.voxelDimension,
                                                                voxelSettingsCurrent.voxelDimension ) );
    VoxelGIPresentationRenderVoxelize( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderVoxelCopy( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderDebug( world, camera, viewWidth, viewHeight, viewHandle );
  }


  void VoxelGIDebugImgui()
  {
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    VoxelSettingsUpdateSerialize();

    if( !ImGuiCollapsingHeader( "Voxel GI Presentation" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;

    ImGuiCheckbox( "Enabled", &voxelSettingsCurrent.voxelEnabled );

    ImGuiCheckbox( "Debug Enabled", &voxelSettingsCurrent.voxelDebug );

    ImGuiCheckbox( "snap voxel grid to camera", &voxelSettingsCurrent.voxelGridSnapCamera );
    ImGuiDragFloat3( "voxel grid center", voxelSettingsCurrent.voxelGridCenter.data() );
    const float oldVoxelGridHalfWidth = voxelSettingsCurrent.voxelGridHalfWidth;
    float width = voxelSettingsCurrent.voxelGridHalfWidth * 2.0f;
    ImGuiDragFloat( "voxel grid width", &width );
    voxelSettingsCurrent.voxelGridHalfWidth = Max( width, 1.0f ) / 2.0f;

/* this is no longer needed, right?
    if( voxelSettingsCurrent.voxelGridHalfWidth != oldVoxelGridHalfWidth )
    {
      SettingsSetNumber( "voxelgi.voxelGridHalfWidth", voxelSettingsCurrent.voxelGridHalfWidth );
    }
    */


    const int oldVoxelDimension = voxelSettingsCurrent.voxelDimension;
    voxelSettingsCurrent.voxelDimension -= ImGuiButton( "-" ) ? 1 : 0;
    ImGuiSameLine();
    voxelSettingsCurrent.voxelDimension += ImGuiButton( "+" ) ? 1 : 0;
    ImGuiSameLine();
    ImGuiDragInt( "voxel dimension", &voxelSettingsCurrent.voxelDimension );
    voxelSettingsCurrent.voxelDimension = Max( voxelSettingsCurrent.voxelDimension, 1 );
    if( oldVoxelDimension != voxelSettingsCurrent.voxelDimension )
    {
      // Destroy old things that need to be resized
      renderDevice->DestroyTexture( voxelTextureRadianceBounce0 );
      renderDevice->DestroyTexture( voxelTextureRadianceBounce1 );
      renderDevice->DestroyBuffer( voxelRWStructuredBuf );
      DestroyVoxelView();

      // Recreate them with the new size
      CreateVoxelTextureRadianceBounce0();
      CreateVoxelTextureRadianceBounce1();
      CreateVoxelRWStructredBuf();
      CreateVoxelView();
    }

    if( voxelSettingsCurrent.voxelDebug )
    {
      ImGuiCheckbox( "Debug draw voxel outlines", &voxelSettingsCurrent.voxelDebugDrawVoxelOutlines );
      ImGuiCheckbox( "Debug draw grid outline", &voxelSettingsCurrent.voxelDebugDrawGridOutline );
      ImGuiCheckbox( "Debug draw voxels", &voxelSettingsCurrent.voxelDebugDrawVoxels );
    }
  }
}

#endif

