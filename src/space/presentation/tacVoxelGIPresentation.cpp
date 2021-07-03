#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacHash.h"
#include "src/common/tacMemory.h"
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
  static int                           voxelDimension = 1; // eventually 128
  static bool                          voxelDebug = true;
  static bool                          voxelDebugDrawVoxelOutlines = false;
  static bool                          voxelDebugDrawGridOutline = true;
  static bool                          voxelDebugDrawVoxels = true;
  static bool                          voxelEnabled = true;
  static v3                            voxelGridCenter;
  static float                         voxelGridHalfWidth = 100.0f;
  static bool                          voxelGridSnapCamera;

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
      texSpec.mImage.mWidth = voxelDimension;
      texSpec.mImage.mHeight = voxelDimension;
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
    texSpec.mAccess = Render::Access::Default; // Render::Access::Dynamic;
    texSpec.mBinding = binding;
    texSpec.mCpuAccess = Render::CPUAccess::None;
    texSpec.mImage.mFormat.mElementCount = 4;
    texSpec.mImage.mFormat.mPerElementDataType = Render::GraphicsType::real;
    texSpec.mImage.mFormat.mPerElementByteCount = 2;
    texSpec.mImage.mWidth = voxelDimension;
    texSpec.mImage.mHeight = voxelDimension;
    texSpec.mImage.mDepth = voxelDimension;
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
    const int voxelCount = voxelDimension * voxelDimension * voxelDimension;
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
    cpuCBufferVoxelizer.gVoxelGridCenter = voxelGridCenter;
    cpuCBufferVoxelizer.gVoxelGridHalfWidth = voxelGridHalfWidth;
    cpuCBufferVoxelizer.gVoxelWidth = ( voxelGridHalfWidth * 2.0f ) / voxelDimension;
    cpuCBufferVoxelizer.gVoxelGridSize = voxelDimension;
    return cpuCBufferVoxelizer;
  }

  static void                    RenderDebugVoxelOutlineGrid( Debug3DDrawData* drawData )
  {
    if( !voxelDebugDrawGridOutline )
      return;
    const v3 mini = voxelGridCenter - voxelGridHalfWidth * v3( 1, 1, 1 );
    const v3 maxi = voxelGridCenter + voxelGridHalfWidth * v3( 1, 1, 1 );
    drawData->DebugDraw3DAABB( mini, maxi );

  }

  static void                    RenderDebugVoxelOutlineVoxels( Debug3DDrawData* drawData )
  {
    if( !voxelDebugDrawVoxelOutlines )
      return;
    TAC_PROFILE_BLOCK;

    for( int i = 0; i < voxelDimension; ++i )
    {
      for( int j = 0; j < voxelDimension; ++j )
      {
        for( int k = 0; k < voxelDimension; ++k )
        {
          const float voxelWidth = ( voxelGridHalfWidth * 2.0f ) / voxelDimension;
          const v3 iVoxel( ( float )i, ( float )j, ( float )k );
          const v3 minPos = voxelGridCenter - voxelGridHalfWidth * v3( 1, 1, 1 ) + voxelWidth * iVoxel;
          const v3 maxPos = minPos + voxelWidth * v3( 1, 1, 1 );
          const v3 minColor = iVoxel / ( float )voxelDimension;
          const v3 maxColor = ( iVoxel + v3( 1, 1, 1 ) ) / ( float )voxelDimension;
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
    if( !voxelDebugDrawVoxels )
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
    Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, voxelDimension * voxelDimension * voxelDimension );
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
    if( !voxelDebug )
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

        // was going to calculate triangle normals here to store in some buffer to feed to the
        // vertex shader, so we dont need the geometry shader so we can debug the vertex shader
        // since you cant debug geometry shaders
        //std::map< HashedValue, int > foo;


        Render::BeginGroup( FrameMemoryPrintf( "%s %i",
                                               model->mModelPath.c_str(),
                                               model->mModelIndex ), TAC_STACK_FRAME );

        DefaultCBufferPerObject objBuf;
        objBuf.Color = { model->mColorRGB, 1 };
        objBuf.World = model->mEntity->mWorldTransform;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
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
          //Render::Submit( mViewHandle, TAC_STACK_FRAME );
          Render::Submit( voxelView, TAC_STACK_FRAME );
        }
        Render::EndGroup( TAC_STACK_FRAME );
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
    Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, voxelDimension * voxelDimension * voxelDimension );
    Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetDepthState( voxelCopyDepthState );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
    Render::SetPixelShaderUnorderedAccessView( voxelTextureRadianceBounce0, 1 );
    Render::Submit( Render::ViewHandle(), TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }


  void VoxelGIPresentationInit( Errors& )
  {
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
    if( !voxelEnabled )
      return;
    voxelGridCenter = voxelGridSnapCamera ? camera->mPos : voxelGridCenter;
    Render::SetViewFramebuffer( voxelView, voxelFramebuffer );
    Render::SetViewport( voxelView, Render::Viewport( voxelDimension, voxelDimension ) );
    Render::SetViewScissorRect( voxelView, Render::ScissorRect( voxelDimension, voxelDimension ) );
    VoxelGIPresentationRenderVoxelize( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderVoxelCopy( world, camera, viewWidth, viewHeight, viewHandle );
    VoxelGIPresentationRenderDebug( world, camera, viewWidth, viewHeight, viewHandle );
  }

  //bool&              VoxelGIPresentationGetEnabled()      { return voxelEnabled; }

  //bool&              VoxelGIPresentationGetDebugEnabled() { return voxelDebug; }

  void VoxelGIDebugImgui()
  {
    if( !ImGuiCollapsingHeader( "Voxel GI Presentation" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;

      //bool& enabled = VoxelGIPresentationGetEnabled();
      //ImGuiCheckbox( "Enabled", &enabled );
    ImGuiCheckbox( "Enabled", &voxelEnabled );

    //bool& debugEnabled = VoxelGIPresentationGetDebugEnabled();
    //ImGuiCheckbox( "Debug Enabled", &debugEnabled );
    ImGuiCheckbox( "Debug Enabled", &voxelDebug );

    ImGuiCheckbox( "snap voxel grid to camera", &voxelGridSnapCamera );
    ImGuiDragFloat3( "voxel grid center", voxelGridCenter.data() );
    float width = voxelGridHalfWidth * 2.0f;
    ImGuiDragFloat( "voxel grid width", &width );
    voxelGridHalfWidth = Max( width, 1.0f ) / 2.0f;

    const int oldVoxelDimension = voxelDimension;
    voxelDimension -= ImGuiButton( "-" ) ? 1 : 0;
    ImGuiSameLine();
    voxelDimension += ImGuiButton( "+" ) ? 1 : 0;
    ImGuiSameLine();
    ImGuiDragInt( "voxel dimension", &voxelDimension );
    voxelDimension = Max( voxelDimension, 1 );
    if( oldVoxelDimension != voxelDimension )
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

    if( voxelDebug )
    {
      ImGuiCheckbox( "Debug draw voxel outlines", &voxelDebugDrawVoxelOutlines );
      ImGuiCheckbox( "Debug draw grid outline", &voxelDebugDrawGridOutline );
      ImGuiCheckbox( "Debug draw voxels", &voxelDebugDrawVoxels );
    }
  }
}


