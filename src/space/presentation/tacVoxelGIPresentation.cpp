#include "src/common/tacCamera.h"
#include "src/common/tacMemory.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/space/graphics/tacgraphics.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/model/tacmodel.h"
#include "src/space/tacentity.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/graphics/tacUI2D.h"

namespace Tac
{
  // https://docs.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-advanced-stages-cs-resources
  // RWStructuredBuffer

  static Render::MagicBufferHandle    voxelRWStructuredBuf;
  static Render::TextureHandle        voxelTextureRadianceBounce0;
  static Render::TextureHandle        voxelTextureRadianceBounce1;
  static Render::VertexFormatHandle   voxelVertexFormat;
  static Render::ShaderHandle         voxelizerShader;
  static Render::ShaderHandle         voxelVisualizerShader;
  static Render::ShaderHandle         voxelCopyShader;
  static Render::ConstantBufferHandle voxelConstantBuffer;
  Render::VertexDeclarations          voxelVertexDeclarations;
  static int                          voxelDimension = 64; // 512; that makes the rwbuffer 1 gb
  static bool                         voxelDebug = true;
  static bool                         voxelEnabled = true;


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



  static void                    CreateVoxelConstantBuffer()
  {
    voxelConstantBuffer = Render::CreateConstantBuffer( sizeof( CBufferVoxelizer ),
                                                        CBufferVoxelizer::shaderregister,
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
    voxelRWStructuredBuf = Render::CreateMagicBuffer( voxelStride * voxelCount,
                                                      voxelStride,
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

  void               VoxelGIPresentationInit( Errors& )
  {
    CreateVoxelizerShader();
    CreateVoxelConstantBuffer();
    CreateVoxelVisualizerShader();
    CreateVoxelCopyShader();
    CreateVoxelRWStructredBuf();
    CreateVoxelTextureRadianceBounce1();
    CreateVoxelTextureRadianceBounce0();
    CreateVertexFormat();
  }

  void               VoxelGIPresentationUninit()
  {

  }

//   WorldVoxelGIState* VoxelGIPresentationCreateState( World* )
//   {
//     auto worldVoxelGIState = TAC_NEW WorldVoxelGIState;
//     return worldVoxelGIState;
//   }


  void               VoxelGIPresentationRenderDebug( World* world,
                                                     const Camera* camera,
                                                     const int viewWidth,
                                                     const int viewHeight,
                                                     const Render::ViewHandle viewHandle )
  {
    //Render::SubmitFrame(); // temp
    //Render::SubmitFrame(); // temp

    { // Temp scope for block

      TAC_RENDER_GROUP_BLOCK( "Voxel GI Debug" );


      CBufferVoxelizer cpuCBufferVoxelizer = {};
      cpuCBufferVoxelizer.gVoxelGridCenter = camera->mPos;
      cpuCBufferVoxelizer.gVoxelGridHalfWidth = ( float )( voxelDimension / 2 );
      cpuCBufferVoxelizer.gVoxelWidth = 1;
      cpuCBufferVoxelizer.gVoxelGridSize = voxelDimension;

      Render::UpdateConstantBuffer( voxelConstantBuffer,
                                     &cpuCBufferVoxelizer,
                                     sizeof( CBufferVoxelizer ),
                                     TAC_STACK_FRAME );
      Render::SetShader( voxelVisualizerShader );
      Render::SetTexture( { voxelTextureRadianceBounce0 } );

#if 0
    // not using UAVs in this shader
      Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );
      Render::SetPixelShaderUnorderedAccessView( voxelTextureRadianceBounce0, 1 );
#endif
      Render::SetPrimitiveTopology( Render::PrimitiveTopology::PointList );
      Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, voxelDimension * voxelDimension * voxelDimension );
      Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
      Render::Submit( viewHandle, TAC_STACK_FRAME );

    }

    //Render::SubmitFrame(); // temp
    //Render::SubmitFrame(); // temp
  }

  void               VoxelGIPresentationRender( World* world,
                                                const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight,
                                                const Render::ViewHandle viewHandle )
  {
    if( !voxelEnabled )
      return;

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

        Render::BeginGroup( FrameMemoryPrintf( "%s %i",
                                               model->mModelPath.c_str(),
                                               model->mModelIndex ), TAC_STACK_FRAME );

        const Render::DepthStateHandle      depthState = GamePresentationGetDepthState();
        const Render::BlendStateHandle      blendState = GamePresentationGetBlendState();
        const Render::RasterizerStateHandle rasterizerState = GamePresentationGetRasterizerState();
        const Render::SamplerStateHandle    samplerState = GamePresentationGetSamplerState();
        const Render::ConstantBufferHandle  objConstantBuffer = GamePresentationGetPerObj();

        DefaultCBufferPerObject objBuf;
        objBuf.Color = { model->mColorRGB, 1 };
        objBuf.World = model->mEntity->mWorldTransform;

        CBufferVoxelizer cpuCBufferVoxelizer = {};
        cpuCBufferVoxelizer.gVoxelGridCenter = mCamera->mPos;
        cpuCBufferVoxelizer.gVoxelGridHalfWidth = ( float )( voxelDimension / 2 );
        cpuCBufferVoxelizer.gVoxelWidth = 1;
        cpuCBufferVoxelizer.gVoxelGridSize = voxelDimension;
        Render::UpdateConstantBuffer( voxelConstantBuffer,
                                      &cpuCBufferVoxelizer,
                                      sizeof( CBufferVoxelizer ),
                                      TAC_STACK_FRAME );

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          Render::SetShader( voxelizerShader );
          Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, 0 );
          Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
          Render::SetBlendState( blendState );
          Render::SetRasterizerState( rasterizerState );
          Render::SetSamplerState( samplerState );
          Render::SetDepthState( depthState );
          Render::SetVertexFormat( voxelVertexFormat );
          Render::SetTexture( { gUI2DCommonData.m1x1White } );
          Render::UpdateConstantBuffer( objConstantBuffer,
                                        &objBuf,
                                        sizeof( DefaultCBufferPerObject ),
                                        TAC_STACK_FRAME );

          Render::SetPixelShaderUnorderedAccessView( voxelRWStructuredBuf, 0 );


          Render::Submit( mViewHandle, TAC_STACK_FRAME );

          //Render::SetMagicBuffer();
        }
        Render::EndGroup( TAC_STACK_FRAME );
      }
      Render::ViewHandle mViewHandle;
      const Camera*      mCamera;
    } modelVisitor;
    modelVisitor.mViewHandle = viewHandle;
    modelVisitor.mCamera = camera;

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
}


