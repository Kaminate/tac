#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacHash.h"
#include "src/common/tacMemory.h"
#include "src/common/tacSettings.h"
#include "src/space/graphics/tacgraphics.h"
#include "src/space/model/tacmodel.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"
#include "src/space/tacentity.h"
#include "src/space/tacworld.h"

#if _MSC_VER
#pragma warning( disable: 4505)
#endif

namespace Tac
{
  // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
  // RWStructuredBuffer

  static Render::MagicBufferHandle     voxelRWStructuredBuf;
  static Render::TextureHandle         voxelTextureRadianceBounce0;
  static Render::TextureHandle         voxelTextureRadianceBounce1;
  static Render::VertexFormatHandle    voxelVertexFormat;
  static Render::ShaderHandle          voxelizerShader;
  static Render::TextureHandle         voxelFramebufferTexture;
  static Render::ShaderHandle          voxelVisualizerShader;
  static Render::ShaderHandle          voxelCopyShader;
  static Render::FramebufferHandle     voxelFramebuffer;
  static Render::ConstantBufferHandle  voxelConstantBuffer;
  static Render::RasterizerStateHandle voxelRasterizerState;
  static Render::DepthStateHandle      voxelCopyDepthState;
  static Render::VertexDeclarations    voxelVertexDeclarations;
  static Render::ViewHandle            voxelView;

  struct VoxelSettings
  {
    int                                voxelDimension = 1; // eventually 128
    bool                               voxelDebug = true;
    bool                               voxelDebugDrawVoxelOutlines = false;
    bool                               voxelDebugDrawGridOutline = true;
    bool                               voxelDebugDrawVoxels = true;
    bool                               voxelEnabled = true;
    v3                                 voxelGridCenter;
    float                              voxelGridHalfWidth = 10.0f;
    bool                               voxelGridSnapCamera;
  };

  static VoxelSettings                 voxelSettingsCurrent;
  static VoxelSettings                 voxelSettingsSaved;
  static double                        voxelSettingLastCheckTime;

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
    uint32_t gVoxelGridSize;

    static const int shaderregister = 2;
  };

  static struct VoxelSettingsSerializer
  {
    struct Number
    {
      const char* GetPath()
      {
        return FrameMemoryPrintf( "voxelgi.%s", mname );
      }

      void        LoadFromSettings( VoxelSettings* voxelSettings )
      {
        void* data = GetData( voxelSettings );
        if( isInt )
          *( int* )data = ( int )SettingsGetNumber( GetPath(), *( int* )data );
        if( isBool )
          *( bool* )data = ( bool )SettingsGetNumber( GetPath(), *( bool* )data );
        if( isFloat )
          *( float* )data = ( float )SettingsGetNumber( GetPath(), *( float* )data );
      }

      void        SaveToSettings( VoxelSettings* voxelSettings )
      {
        void* data = GetData( voxelSettings );
        if( isInt )
          SettingsSetNumber( GetPath(), ( JsonNumber )*( int* )data );
        if( isBool )
          SettingsSetNumber( GetPath(), ( JsonNumber )*( bool* )data );
        if( isFloat )
          SettingsSetNumber( GetPath(), ( JsonNumber )*( float* )data );
      }

      void*       GetData( VoxelSettings* voxelSettings )
      {
        return  ( char* )voxelSettings + offset;
      }

      int         offset = 0;
      bool        isBool = 0;
      bool        isInt = 0;
      bool        isFloat = 0;
      const char* mname = nullptr;
    };

    Number*                     AddNumber( int offset, const char* name )
    {
      Number number = {};
      number.offset = offset;
      number.mname = name;
      numbers.push_back( number );
      return &numbers.back();
    }
    template< typename T > void AddType( int offset, const char* name ) = delete;
    template<> void             AddType< bool >( int offset, const char* name )  { AddBool( offset, name ); }
    template<> void             AddType< int >( int offset, const char* name )   { AddInt( offset, name ); }
    template<> void             AddType< float >( int offset, const char* name ) { AddFloat( offset, name ); }
    void                        AddBool( int offset, const char* name )  { AddNumber( offset, name )->isBool = true; }
    void                        AddFloat( int offset, const char* name ) { AddNumber( offset, name )->isFloat = true; }
    void                        AddInt( int offset, const char* name )   { AddNumber( offset, name )->isInt = true; }

    VoxelSettingsSerializer()
    {
#define REGISTER_VAR( var ) AddType< decltype( VoxelSettings::var ) >( TAC_OFFSET_OF( VoxelSettings, var ), TAC_STRINGIFY( var ) );

      REGISTER_VAR( voxelDimension );
      REGISTER_VAR( voxelDebug );
      REGISTER_VAR( voxelDebugDrawVoxelOutlines );
      REGISTER_VAR( voxelDebugDrawGridOutline );
      REGISTER_VAR( voxelDebugDrawVoxels );
      REGISTER_VAR( voxelEnabled );
      REGISTER_VAR( voxelGridCenter.x );
      REGISTER_VAR( voxelGridCenter.y );
      REGISTER_VAR( voxelGridCenter.z );
      REGISTER_VAR( voxelGridHalfWidth );
      REGISTER_VAR( voxelGridSnapCamera );
    }


    Vector< Number > numbers;
  } voxelSettingsSerializer;

  static bool                    VoxelSettingsChanged( VoxelSettings* a, VoxelSettings* b )
  {
    for( auto& number : voxelSettingsSerializer.numbers )
    {
      if( number.isBool && *( bool* )number.GetData( a ) != *( bool* )number.GetData( b ) )
          return true;

      if( number.isInt && *( int* )number.GetData( a ) != *( int* )number.GetData( b ) )
          return true;

      if( number.isFloat && *( float* )number.GetData( a ) != *( float* )number.GetData( b ) )
          return true;
    }
    return false;
  }

  static void                    VoxelSettingsSave( VoxelSettings* voxelSettings )
  {
    for( auto& number : voxelSettingsSerializer.numbers )
      number.SaveToSettings( voxelSettings );
  }

  static void                    VoxelSettingsLoad( VoxelSettings* voxelSettings )
  {
    for( auto& number : voxelSettingsSerializer.numbers )
      number.LoadFromSettings( voxelSettings );
  }


  static Render::ConstantBuffers GetConstantBuffers()
  {
    return
    {
      GamePresentationGetPerFrame(),
      GamePresentationGetPerObj(),
    };
  }

  static void                    CreateVoxelDepthState()
  {
    voxelCopyDepthState = Render::CreateDepthState( {}, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelCopyDepthState, "vox-copy-depth" );
  }

  static void                    CreateVoxelView()
  {
    voxelFramebufferTexture = []()
    {
      // The framebuffer texture is only to allow for renderdoc debugging pixel shaders
      Render::TexSpec texSpec;
      texSpec.mImage.mWidth = voxelSettingsCurrent.voxelDimension;
      texSpec.mImage.mHeight = voxelSettingsCurrent.voxelDimension;
      texSpec.mImage.mFormat.mElementCount = 4;
      texSpec.mImage.mFormat.mPerElementByteCount = 1;
      texSpec.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
      texSpec.mBinding = Render::Binding::RenderTarget;
      auto tex = Render::CreateTexture( texSpec, TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( tex, "voxel-fbo-tex" );
      return tex;
    }( );

    voxelFramebuffer = [](){
      Render::FramebufferTextures framebufferTextures = { voxelFramebufferTexture };
      auto fbo = Render::CreateFramebufferForRenderToTexture( framebufferTextures, TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( fbo, "voxel-fbo" );
      return fbo;
    }( );

    voxelView = Render::CreateView();
  }

  static void                    DestroyVoxelView()
  {
    Render::DestroyTexture( voxelFramebufferTexture, TAC_STACK_FRAME );
    Render::DestroyFramebuffer( voxelFramebuffer, TAC_STACK_FRAME );
    Render::DestroyView( voxelView );
  }

  static void                    CreateVoxelConstantBuffer()
  {
    voxelConstantBuffer = Render::CreateConstantBuffer( sizeof( CBufferVoxelizer ),
                                                        CBufferVoxelizer::shaderregister,
                                                        TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelConstantBuffer, "vox-constant-buf" );
  }

  static void                    CreateVoxelRasterizerState()
  {
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None;
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = false;
    rasterizerStateData.mConservativeRasterization = true;
    voxelRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                          TAC_STACK_FRAME );
  }

  static void                    CreateVoxelVisualizerShader()
  {
    // This shader is used to debug visualize the voxel radiance
    voxelVisualizerShader = Render::CreateShader( Render::ShaderSource::FromPath( "VoxelVisualizer" ),
                                                  GetConstantBuffers(),
                                                  TAC_STACK_FRAME );
  }

  static void                    CreateVoxelCopyShader()
  {
    // This shader is used to copy from the 3d magic buffer to the 3d texture
    voxelCopyShader = Render::CreateShader( Render::ShaderSource::FromPath( "VoxelCopy" ),
                                            GetConstantBuffers(),
                                            TAC_STACK_FRAME );
  }

  static void                    CreateVoxelizerShader()
  {
    // The voxelizer shader turns geometry into a rwstructuredbuffer using atomics
    // to prevent flickering
    voxelizerShader = Render::CreateShader( Render::ShaderSource::FromPath( "Voxelizer" ),
                                            GetConstantBuffers(),
                                            TAC_STACK_FRAME );
  }

  static void                    CreateVertexFormat()
  {
    struct VoxelVtx
    {
      v3 pos;
      v3 nor;
      v2 uv;
    };

    Render::VertexDeclaration pos;
    pos.mAlignedByteOffset = TAC_OFFSET_OF( VoxelVtx, pos );
    pos.mAttribute = Render::Attribute::Position;
    pos.mTextureFormat.mElementCount = 3;
    pos.mTextureFormat.mPerElementByteCount = sizeof( float );
    pos.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;

    Render::VertexDeclaration nor;
    nor.mAlignedByteOffset = TAC_OFFSET_OF( VoxelVtx, nor );
    nor.mAttribute = Render::Attribute::Normal;
    nor.mTextureFormat.mElementCount = 3;
    nor.mTextureFormat.mPerElementByteCount = sizeof( float );
    nor.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;

    Render::VertexDeclaration uv;
    uv.mAlignedByteOffset = TAC_OFFSET_OF( VoxelVtx, uv );
    uv.mAttribute = Render::Attribute::Texcoord;
    uv.mTextureFormat.mElementCount = 2;
    uv.mTextureFormat.mPerElementByteCount = sizeof( float );
    uv.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;

    voxelVertexDeclarations = { pos, nor, uv };

    voxelVertexFormat = Render::CreateVertexFormat( voxelVertexDeclarations,
                                                    voxelizerShader,
                                                    TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelVertexFormat, "vox-vtx-fmt" );
  }

  static Render::TexSpec         GetVoxRadianceTexSpec()
  {
    const auto binding = ( Render::Binding ) (
      ( int )Render::Binding::ShaderResource |
      ( int )Render::Binding::UnorderedAccess );

    // rgba16f, 2 bytes (16 bits) per float, hdr values
    Render::TexSpec texSpec;
    texSpec.mAccess = Render::Access::Default;
    texSpec.mBinding = binding;
    texSpec.mCpuAccess = Render::CPUAccess::None;
    texSpec.mImage.mFormat.mElementCount = 4;
    texSpec.mImage.mFormat.mPerElementDataType = Render::GraphicsType::real;
    texSpec.mImage.mFormat.mPerElementByteCount = 2;
    texSpec.mImage.mWidth = voxelSettingsCurrent.voxelDimension;
    texSpec.mImage.mHeight = voxelSettingsCurrent.voxelDimension;
    texSpec.mImage.mDepth = voxelSettingsCurrent.voxelDimension;
    texSpec.mPitch = 0;
    return texSpec;
  }

  static void                    CreateVoxelTextureRadianceBounce1()
  {
    voxelTextureRadianceBounce1 = Render::CreateTexture( GetVoxRadianceTexSpec(), TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelTextureRadianceBounce1, "radiance-bounce-1" );
  }

  static void                    CreateVoxelTextureRadianceBounce0()
  {
    voxelTextureRadianceBounce0 = Render::CreateTexture( GetVoxRadianceTexSpec(), TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( voxelTextureRadianceBounce0, "radiance-bounce-0" );
  }

  static void                    CreateVoxelRWStructredBuf()
  {
    const int voxelStride = sizeof( uint32_t ) * 2;
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
    CBufferVoxelizer cpuCBufferVoxelizer = {};
    cpuCBufferVoxelizer.gVoxelGridCenter = voxelSettingsCurrent.voxelGridCenter;
    cpuCBufferVoxelizer.gVoxelGridHalfWidth = voxelSettingsCurrent.voxelGridHalfWidth;
    cpuCBufferVoxelizer.gVoxelWidth = ( voxelSettingsCurrent.voxelGridHalfWidth * 2.0f ) / voxelSettingsCurrent.voxelDimension;
    cpuCBufferVoxelizer.gVoxelGridSize = voxelSettingsCurrent.voxelDimension;
    return cpuCBufferVoxelizer;
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
          const float voxelWidth = ( voxelSettingsCurrent.voxelGridHalfWidth * 2.0f ) / voxelSettingsCurrent.voxelDimension;
          const v3 iVoxel( ( float )i, ( float )j, ( float )k );
          const v3 minPos = voxelSettingsCurrent.voxelGridCenter - voxelSettingsCurrent.voxelGridHalfWidth * v3( 1, 1, 1 ) + voxelWidth * iVoxel;
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
    Render::UpdateConstantBuffer( voxelConstantBuffer,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );
    Render::SetShader( voxelVisualizerShader );
    Render::SetDepthState( GamePresentationGetDepthState() );
    Render::SetTexture( { voxelTextureRadianceBounce0 } );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetVertexBuffer( Render::VertexBufferHandle(),
                             0,
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension );
    Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
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

    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             voxelVertexDeclarations,
                                                             errors );
        if( !mesh )
          return;

        DefaultCBufferPerObject objBuf;
        objBuf.Color = { model->mColorRGB, 1 };
        objBuf.World = model->mEntity->mWorldTransform;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          Render::BeginGroup( subMesh.mName, TAC_STACK_FRAME );
          Render::SetShader( voxelizerShader );
          Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
          Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
          Render::SetBlendState( mBlendState );
          Render::SetRasterizerState( voxelRasterizerState );
          Render::SetSamplerState( mSamplerState );
          Render::SetDepthState( mDepthState );
          Render::SetVertexFormat( voxelVertexFormat );
          Render::SetTexture( { mTexture } );
          Render::UpdateConstantBuffer( mObjConstantBuffer,
                                        &objBuf,
                                        sizeof( DefaultCBufferPerObject ),
                                        TAC_STACK_FRAME );
          Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
          Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
          Render::Submit( voxelView, TAC_STACK_FRAME );
          Render::EndGroup( TAC_STACK_FRAME );
        }
      }

      Render::ViewHandle            mViewHandle;
      Render::DepthStateHandle      mDepthState;
      Render::BlendStateHandle      mBlendState;
      Render::SamplerStateHandle    mSamplerState;
      Render::ConstantBufferHandle  mObjConstantBuffer;
      Render::TextureHandle         mTexture;
    } modelVisitor;
    modelVisitor.mViewHandle = viewHandle;
    modelVisitor.mDepthState = GamePresentationGetDepthState();
    modelVisitor.mBlendState = GamePresentationGetBlendState();
    modelVisitor.mSamplerState = GamePresentationGetSamplerState();
    modelVisitor.mObjConstantBuffer = GamePresentationGetPerObj();
    modelVisitor.mTexture = Get1x1White();

    const CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();
    Render::UpdateConstantBuffer( voxelConstantBuffer,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );

    const Graphics* graphics = GetGraphics( world );
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
    Render::SetVertexBuffer( Render::VertexBufferHandle(), 0,
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension *
                             voxelSettingsCurrent.voxelDimension );
    Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetDepthState( voxelCopyDepthState );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
    Render::SetPixelShaderUnorderedAccessView( voxelTextureRadianceBounce0, 1 );
    Render::Submit( Render::ViewHandle(), TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }

  // Imagine if all this shit can be replaced with
  // TAC_TWEAK_VAR( &voxelSettings.foo )
  // which automatically does the load, the check time and saving

  static void                    VoxelSettingsUpdateSerialize()
  {
    if( voxelSettingLastCheckTime + 0.1f > ShellGetElapsedSeconds() )
      return;

    voxelSettingLastCheckTime = ShellGetElapsedSeconds();
    if( !VoxelSettingsChanged( &voxelSettingsCurrent, &voxelSettingsSaved ) )
      return;

    VoxelSettingsSave( &voxelSettingsCurrent );
    voxelSettingsSaved = voxelSettingsCurrent;
  }

  void VoxelGIPresentationInit( Errors& )
  {
    VoxelSettingsLoad( &voxelSettingsSaved );
    voxelSettingsCurrent = voxelSettingsSaved;
    voxelSettingLastCheckTime = ShellGetElapsedSeconds();

    CreateVoxelizerShader();
    CreateVoxelConstantBuffer();
    CreateVoxelView();
    CreateVoxelRasterizerState();
    CreateVoxelVisualizerShader();
    CreateVoxelCopyShader();
    CreateVoxelRWStructredBuf();
    CreateVoxelTextureRadianceBounce0();
    CreateVoxelTextureRadianceBounce1();
    CreateVertexFormat();
    CreateVoxelDepthState();
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
    voxelSettingsCurrent.voxelGridCenter = voxelSettingsCurrent.voxelGridSnapCamera ? camera->mPos : voxelSettingsCurrent.voxelGridCenter;
    Render::SetViewFramebuffer( voxelView, voxelFramebuffer );
    Render::SetViewport( voxelView, Render::Viewport( voxelSettingsCurrent.voxelDimension, voxelSettingsCurrent.voxelDimension ) );
    Render::SetViewScissorRect( voxelView, Render::ScissorRect( voxelSettingsCurrent.voxelDimension, voxelSettingsCurrent.voxelDimension ) );
    VoxelGIPresentationRenderVoxelize( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderVoxelCopy( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderDebug( world, camera, viewWidth, viewHeight, viewHandle );
  }


  void VoxelGIDebugImgui()
  {
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
    if( voxelSettingsCurrent.voxelGridHalfWidth != oldVoxelGridHalfWidth )
    {
      SettingsSetNumber( "voxelgi.voxelGridHalfWidth", voxelSettingsCurrent.voxelGridHalfWidth );
    }


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
      Render::DestroyTexture( voxelTextureRadianceBounce0, TAC_STACK_FRAME );
      Render::DestroyTexture( voxelTextureRadianceBounce1, TAC_STACK_FRAME );
      Render::DestroyMagicBuffer( voxelRWStructuredBuf, TAC_STACK_FRAME );
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


