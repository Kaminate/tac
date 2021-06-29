#include "src/common/assetmanagers/tacMesh.h"
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

namespace Tac
{
  // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
  // RWStructuredBuffer

  static Render::MagicBufferHandle     voxelRWStructuredBuf;
  static Render::TextureHandle         voxelTextureRadianceBounce0;
  static Render::TextureHandle         voxelTextureRadianceBounce1;
  static Render::VertexFormatHandle    voxelVertexFormat;
  static Render::ShaderHandle          voxelizerShader;
  static Render::ShaderHandle          voxelVisualizerShader;

  // use null ID3D11InputLayout* instead
  //static Render::VertexFormatHandle    voxelVisualizerEmptyInputLayout;

  static Render::ShaderHandle          voxelCopyShader;
  static Render::ConstantBufferHandle  voxelConstantBuffer;
  static Render::RasterizerStateHandle voxelRasterizerState;

  Render::VertexDeclarations           voxelVertexDeclarations;
  static int                           voxelDimension = 2;
  static bool                          voxelDebug = true;
  static bool                          voxelEnabled = true;
  static v3                            voxelGridCenter;
  static float                         voxelGridHalfWidth = 2.0f;
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

  //static void                    CreateVoxelVisualizerInputLayout()
  //{
  //  voxelVisualizerEmptyInputLayout = Render::CreateVertexFormat( {}, voxelVisualizerShader, TAC_STACK_FRAME );
  //}

  static void                    CreateVoxelConstantBuffer()
  {
    voxelConstantBuffer = Render::CreateConstantBuffer( sizeof( CBufferVoxelizer ),
                                                        CBufferVoxelizer::shaderregister,
                                                        TAC_STACK_FRAME );
  }

  static void                    CreateVoxelRasterizerState()
  {
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None;
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
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
  }

  static Render::TexSpec         GetVoxRadianceTexSpec()
  {
    const auto binding = ( Render::Binding ) (
      ( int )Render::Binding::ShaderResource |
      ( int )Render::Binding::UnorderedAccess );

    // rgb16f, 2 bytes (16 bits) per float, hdr values
    Render::TexSpec texSpec;
    texSpec.mAccess = Render::Access::Default; // Render::Access::Dynamic;
    texSpec.mBinding = binding;
    texSpec.mCpuAccess = Render::CPUAccess::None;
    texSpec.mImage.mFormat.mElementCount = 3;
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
  }

  static void                    CreateVoxelTextureRadianceBounce0()
  {
    voxelTextureRadianceBounce0 = Render::CreateTexture( GetVoxRadianceTexSpec(), TAC_STACK_FRAME );
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



             //TAC_CRITICAL_ERROR_UNIMPLEMENTED;

             //GPUBufferDesc desc;
             //desc.StructureByteStride = sizeof( uint32_t ) * 2;
             //desc.ByteWidth = desc.StructureByteStride * voxelSceneData.res * voxelSceneData.res * voxelSceneData.res;
             //desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
             //desc.CPUAccessFlags = 0;
             //desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
             //desc.Usage = USAGE_DEFAULT;

             //in wicked engine, the structured buffer is created with a CreateBuffer() function,
             //  which is different from the CreateTexture() function!

             //  eventually fed into ID3D11Device::CreateBuffer 

             //voxTexScene....;
             // must be integral type for shader atomics

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

  static void                    DrawRubixCubeSide( int iAxis, Debug3DDrawData* drawData )
  {
    const v3 x( 1, 0, 0 );
    const v3 y( 0, 1, 0 );
    const v3 z( 0, 0, 1 );
    const v3 axes[] = { x,y,z };

    const v3 mainAxis = axes[ iAxis ];
    const v3 perpA = axes[ ( iAxis + 1 ) % 3 ];
    const v3 perpB = axes[ ( iAxis + 2 ) % 3 ];


    CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();
    v3 planeCenter
      = cpuCBufferVoxelizer.gVoxelGridCenter
      - mainAxis * cpuCBufferVoxelizer.gVoxelGridHalfWidth;

    const v3 miniPos = cpuCBufferVoxelizer.gVoxelGridCenter - v3( 1, 1, 1 ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;
    const v3 maxiPos = cpuCBufferVoxelizer.gVoxelGridCenter + v3( 1, 1, 1 ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;

    for( int i = 0; i <= ( int )cpuCBufferVoxelizer.gVoxelGridSize; ++i )
    {
      v3 bl = planeCenter + ( -perpA - perpB ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;
      v3 tl = planeCenter + ( -perpA + perpB ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;
      v3 br = planeCenter + ( perpA - perpB ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;
      v3 tr = planeCenter + ( perpA + perpB ) * cpuCBufferVoxelizer.gVoxelGridHalfWidth;

      for( int j = 0; j <= ( int )cpuCBufferVoxelizer.gVoxelGridSize; ++j )
      {

        auto GetColor = [ & ]( v3 p )
        {
          v3 pColor;
          pColor.x = ( p.x - miniPos.x ) / ( maxiPos.x - miniPos.x );
          pColor.y = ( p.y - miniPos.y ) / ( maxiPos.y - miniPos.y );
          pColor.z = ( p.z - miniPos.z ) / ( maxiPos.z - miniPos.z );
          return pColor;
        };

        float lerpT = ( float )j / ( float )cpuCBufferVoxelizer.gVoxelGridSize;
        v3 b = Lerp( bl, br, lerpT );
        v3 t = Lerp( tl, tr, lerpT );
        drawData->DebugDraw3DLine( b, t, GetColor( b ), GetColor( t ) );
        v3 l = Lerp( bl, tl, lerpT );
        v3 r = Lerp( br, tr, lerpT );
        drawData->DebugDraw3DLine( l, r, GetColor( l ), GetColor( r ) );

      }

      planeCenter += mainAxis * cpuCBufferVoxelizer.gVoxelWidth;
    }
  }

  static void                    DrawRubixCube( Debug3DDrawData* drawData )
  {
    for( int iAxis = 0; iAxis < 3; ++iAxis )
      DrawRubixCubeSide( iAxis, drawData );
  }

  void               VoxelGIPresentationInit( Errors& )
  {
    CreateVoxelizerShader();
    CreateVoxelConstantBuffer();
    CreateVoxelRasterizerState();
    CreateVoxelVisualizerShader();
    //CreateVoxelVisualizerInputLayout();
    CreateVoxelCopyShader();
    CreateVoxelRWStructredBuf();
    CreateVoxelTextureRadianceBounce1();
    CreateVoxelTextureRadianceBounce0();
    CreateVertexFormat();
  }

  void               VoxelGIPresentationUninit()
  {

  }

  void               VoxelGIPresentationRenderDebug( World* world,
                                                     const Camera* camera,
                                                     const int viewWidth,
                                                     const int viewHeight,
                                                     const Render::ViewHandle viewHandle )
  {
    //Render::SubmitFrame(); // temp
    //Render::SubmitFrame(); // temp

    Render::BeginGroup( "Voxel GI Debug", TAC_STACK_FRAME );

    CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();

    Render::UpdateConstantBuffer( voxelConstantBuffer,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );
    Render::SetShader( voxelVisualizerShader );
    Render::SetTexture( { voxelTextureRadianceBounce0 } );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
    Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, voxelDimension * voxelDimension * voxelDimension );
    Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
    Render::Submit( viewHandle, TAC_STACK_FRAME );

    Render::EndGroup( TAC_STACK_FRAME );

    //Render::SubmitFrame(); // temp
    //Render::SubmitFrame(); // temp


    DrawRubixCube( world->mDebug3DDrawData );

  }

  void               VoxelGIPresentationRender( World* world,
                                                const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight,
                                                const Render::ViewHandle viewHandle )
  {
    if( !voxelEnabled )
      return;

    voxelGridCenter = voxelGridSnapCamera ? camera->mPos : voxelGridCenter;

    Render::BeginGroup( "Voxel GI", TAC_STACK_FRAME );

    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        TAC_UNUSED_PARAMETER( model );


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
        std::map< HashedValue, int > foo;


        Render::BeginGroup( FrameMemoryPrintf( "%s %i",
                                               model->mModelPath.c_str(),
                                               model->mModelIndex ), TAC_STACK_FRAME );

        //const Render::DepthStateHandle      depthState = GamePresentationGetDepthState();
        //const Render::BlendStateHandle      blendState = GamePresentationGetBlendState();
        //const Render::SamplerStateHandle    samplerState = GamePresentationGetSamplerState();
        //const Render::ConstantBufferHandle  objConstantBuffer = GamePresentationGetPerObj();

        DefaultCBufferPerObject objBuf;
        objBuf.Color = { model->mColorRGB, 1 };
        objBuf.World = model->mEntity->mWorldTransform;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          Render::SetShader( voxelizerShader );
          Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
          Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
          Render::SetBlendState( blendState );
          Render::SetRasterizerState( voxelRasterizerState );
          Render::SetSamplerState( samplerState );
          Render::SetDepthState( depthState );
          Render::SetVertexFormat( voxelVertexFormat );
          Render::SetTexture( { texture } );
          Render::UpdateConstantBuffer( objConstantBuffer,
                                        &objBuf,
                                        sizeof( DefaultCBufferPerObject ),
                                        TAC_STACK_FRAME );

          Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
          Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );


          Render::Submit( mViewHandle, TAC_STACK_FRAME );

          //Render::SetMagicBuffer();
        }
        Render::EndGroup( TAC_STACK_FRAME );
      }
      Render::ViewHandle mViewHandle;

      const Render::DepthStateHandle      depthState = GamePresentationGetDepthState();
      const Render::BlendStateHandle      blendState = GamePresentationGetBlendState();
      const Render::SamplerStateHandle    samplerState = GamePresentationGetSamplerState();
      const Render::ConstantBufferHandle  objConstantBuffer = GamePresentationGetPerObj();
      const Render::TextureHandle         texture = Get1x1White();

    } modelVisitor;
    modelVisitor.mViewHandle = viewHandle;

    CBufferVoxelizer cpuCBufferVoxelizer = VoxelGetCBuffer();
    Render::UpdateConstantBuffer( voxelConstantBuffer,
                                  &cpuCBufferVoxelizer,
                                  sizeof( CBufferVoxelizer ),
                                  TAC_STACK_FRAME );


    Graphics* graphics = GetGraphics( world );

    graphics->VisitModels( &modelVisitor );

    //TAC_UNUSED_PARAMETER( world );
    //TAC_UNUSED_PARAMETER( camera );
    //TAC_UNUSED_PARAMETER( viewWidth );
    //TAC_UNUSED_PARAMETER( viewHeight );
    //TAC_UNUSED_PARAMETER( viewHandle );

    Render::EndGroup( TAC_STACK_FRAME );

    if( voxelDebug )
      VoxelGIPresentationRenderDebug( world, camera, viewWidth, viewHeight, viewHandle );
  }

  bool&              VoxelGIPresentationGetEnabled()      { return voxelEnabled; }

  bool&              VoxelGIPresentationGetDebugEnabled() { return voxelDebug; }

  void               VoxelDebugImgui()
  {
    ImGuiCheckbox( "snap voxel grid to camera", &voxelGridSnapCamera );
    ImGuiDragFloat3( "voxel grid center", voxelGridCenter.data() );
    float width = voxelGridHalfWidth * 2.0f;
    ImGuiDragFloat( "voxel grid width", &width );
    voxelGridHalfWidth = Max( width, 1.0f ) / 2.0f;

    ImGuiDragInt( "voxel dimension", &voxelDimension );
    voxelDimension = Max( voxelDimension, 1 );
  }
}


