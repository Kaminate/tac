#include "src/common/tacCamera.h"
#include "src/common/tacMemory.h"
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

  static Render::TextureHandle      voxTexScene;
  //static Render::TextureHandle      voxTexRadianceBounce0;
  //static Render::TextureHandle      voxTexRadianceBounce1;
  static Render::VertexFormatHandle vertexFormat;
  static Render::ShaderHandle       voxShader;
  Render::VertexDeclarations        vertexDeclarations;

  static void CreateVoxShader()
  {
    const Render::ConstantBuffers constantBuffers =
    {
      GamePresentationGetPerFrame(),
      GamePresentationGetPerObj(),
    };

    voxShader = Render::CreateShader( Render::ShaderSource::FromPath( "Voxelizer" ),
                                      constantBuffers,
                                      TAC_STACK_FRAME );
  }

  static void CreateVertexFormat()
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
    nor.mAlignedByteOffset = 0;
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

    vertexDeclarations = Render::VertexDeclarations{ pos, nor, uv };

    vertexFormat = Render::CreateVertexFormat( vertexDeclarations,
                                               voxShader,
                                               TAC_STACK_FRAME );
  }

  static void CreateVoxTex()
  {
    Render::TexSpec texSpec;
    texSpec.mAccess = Render::Access::Dynamic;
    texSpec.mBinding = ( Render::Binding ) (
      ( int )Render::Binding::ShaderResource |
      ( int )Render::Binding::UnorderedAccess );
    texSpec.mCpuAccess = Render::CPUAccess::None;
    // The .mImage is unused in a structured buffer
    //texSpec.mImage.mFormat = ;
    //texSpec.mImage.mWidth = ;
    //texSpec.mImage.mHeight = ;
    //texSpec.mPitch = ;
    voxTexScene = Render::CreateTexture( texSpec, TAC_STACK_FRAME );

    TAC_CRITICAL_ERROR_UNIMPLEMENTED;

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
    CreateVoxShader();
    CreateVoxTex();
    CreateVertexFormat();
  }

  void               VoxelGIPresentationUninit()
  {
    static int asdf;
    ++asdf;

  }

//   WorldVoxelGIState* VoxelGIPresentationCreateState( World* )
//   {
//     auto worldVoxelGIState = TAC_NEW WorldVoxelGIState;
//     return worldVoxelGIState;
//   }


  void               VoxelGIPresentationRender( World* world,
                                                const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight,
                                                const Render::ViewHandle viewHandle )
  {
    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        TAC_UNUSED_PARAMETER( model );


        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             vertexDeclarations,
                                                             errors );
        if( !mesh )
          return;


        const Render::DepthStateHandle      depthState = GamePresentationGetDepthState();
        const Render::BlendStateHandle      blendState = GamePresentationGetBlendState();
        const Render::RasterizerStateHandle rasterizerState = GamePresentationGetRasterizerState();
        const Render::SamplerStateHandle    samplerState = GamePresentationGetSamplerState();
        const Render::ConstantBufferHandle  objConstantBuffer = GamePresentationGetPerObj();

        DefaultCBufferPerObject objBuf;
        objBuf.Color = { model->mColorRGB, 1 };
        objBuf.World = model->mEntity->mWorldTransform;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          Render::SetShader( voxShader );
          Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, 0 );
          Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
          Render::SetBlendState( blendState );
          Render::SetRasterizerState( rasterizerState );
          Render::SetSamplerState( samplerState );
          Render::SetDepthState( depthState );
          Render::SetVertexFormat( vertexFormat );
          //Render::SetTexture( { gUI2DCommonData.m1x1White } );
          Render::UpdateConstantBuffer( objConstantBuffer,
                                        &objBuf,
                                        sizeof( DefaultCBufferPerObject ),
                                        TAC_STACK_FRAME );
          Render::Submit( mViewHandle, TAC_STACK_FRAME );
        }
      }
      Render::ViewHandle mViewHandle;
    } modelVisitor;
    modelVisitor.mViewHandle = viewHandle;

    Graphics* graphics = GetGraphics( world );
    graphics->VisitModels( &modelVisitor );

    TAC_UNUSED_PARAMETER( world );
    TAC_UNUSED_PARAMETER( camera );
    TAC_UNUSED_PARAMETER( viewWidth );
    TAC_UNUSED_PARAMETER( viewHeight );
    TAC_UNUSED_PARAMETER( viewHandle );
  }
}


