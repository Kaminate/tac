// renderer interface
// used for creating the things necessary to put pretty pixels on the screen
// ( textures, shaders, geometry ( vertexes + indexes ) )

#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacPreprocessor.h"
#include <mutex>
#include <set>


// ok so like
// there will be a render thread
// there will be a submit thread
// and then like theres worker threads
//
// the render thread uses render resources
// the submit thread uses submit resources
// the worker threads use encoders.
// the worker thread encoders use a encoder semaphore


namespace Tac
{
  struct DesktopWindow;
  struct Renderer;
  struct CBuffer;
  struct Shell;



  const v4 colorGrey = v4( v3( 1, 1, 1 ) * 95.0f, 255 ) / 255.0f;
  const v4 colorOrange = v4( 255, 200, 84, 255 ) / 255.0f;
  const v4 colorGreen = v4( 0, 255, 112, 255 ) / 255.0f;
  const v4 colorBlue = v4( 84, 255, 255, 255 ) / 255.0f;
  const v4 colorRed = v4( 255, 84, 84, 255 ) / 255.0f;
  const v4 colorMagenta = v4( 255, 84, 255, 255 ) / 255.0f;




  enum class Attribute // Used to hardcode shader semantics/indexes
  {
    Position,
    Normal,
    Texcoord,
    Color,
    BoneIndex,
    BoneWeight,
    Coeffs,
    Count
  };
  enum class GraphicsType
  {
    unknown,
    sint,
    uint,
    snorm,
    unorm,
    real
  };
  enum class DepthFunc
  {
    Less,
    LessOrEqual,
  };
  enum class AddressMode
  {
    Wrap,
    Clamp,
    Border,
  };
  enum class Comparison
  {
    Always,
    Never,
  };
  enum class Filter
  {
    Point,
    Linear,
    Aniso,
  };
  enum class ShaderType
  {
    Vertex,
    Fragment,
    Count,
  };
  enum class Access
  {
    Static, // Never gonna change
    Default, // ?
    Dynamic, // Gonna change ( debug draw, animation )
  };
  enum class CPUAccess
  {
    Read = 0b01,
    Write = 0b10
  };
  enum class Map
  {
    Read,
    Write,
    ReadWrite,
    WriteDiscard, // the previous contents will be discarded
  };
  enum class Primitive
  {
    TriangleList,
    LineList
  };
  enum class BlendMode
  {
    Add,
  };
  enum class BlendConstants
  {
    One,
    Zero,
    SrcRGB,
    SrcA,
    OneMinusSrcA,
  };
  enum class FillMode
  {
    Solid,
    Wireframe
  };
  enum class CullMode
  {
    None,
    Back,
    Front
  };
  enum class Binding
  {
    ShaderResource = 0b01,
    RenderTarget = 0b10,
  };

  const char* GetSemanticName( Attribute attribType );

  // Used so the gpu can translate from cpu types to gpu types
  struct Format
  {
    int CalculateTotalByteCount() const;
    int mElementCount = 0;
    int mPerElementByteCount = 0;
    GraphicsType mPerElementDataType = GraphicsType::unknown;
  };

  const Format formatv2 = { 2, sizeof( float ), GraphicsType::real };
  const Format formatv3 = { 3, sizeof( float ), GraphicsType::real };


  struct Image
  {
    int mWidth = 0;
    int mHeight = 0;

    // comment?
    int mPitch = 0;
    void* mData = nullptr;
    Format mFormat;
  };
  struct Constant
  {
    String mName;
    int mOffset = 0;
    int mSize = 0;
  };
  struct VertexDeclaration
  {
    Attribute mAttribute = Attribute::Count;
    Format mTextureFormat;

    // Offset of the variable from the vertex buffer
    // ie: OffsetOf( MyVertexType, mPosition)
    int mAlignedByteOffset = 0;
  };




  // don't store these in a Owned, they should be both new'd and delete'd by the renderer
  struct RendererResource
  {
    virtual ~RendererResource()
    {
      // Please call remove renderer resource instead of deleting directly
      TAC_ASSERT( mActive == false );
    }
    bool mActive = false;
    String mName;
    StackFrame mFrame;
  };

  struct ShaderData : public RendererResource
  {
    // can load from either
    String mShaderPath;
    String mShaderStr;

    Vector< CBuffer* > mCBuffers;
  };
  struct Shader : public ShaderData { };// for now, this encompasses both the vertex & pixel shader 
  struct VertexBufferData : public RendererResource
  {
    Access mAccess = Access::Default;
    void* mOptionalData = nullptr;
    int mNumVertexes = 0;
    int mStrideBytesBetweenVertexes = 0;
  };
  struct VertexBuffer : public VertexBufferData
  {
    virtual void Overwrite( void* data, int byteCount, Errors& errors ) { TAC_UNIMPLEMENTED; };
  };
  struct IndexBufferData : public RendererResource
  {
    Access mAccess = Access::Default;
    const void* mData = nullptr;
    int mIndexCount = 0;
    Format mFormat;
  };
  struct IndexBuffer : public IndexBufferData
  {
    virtual void Overwrite( void* data, int byteCount, Errors& errors ) { TAC_UNIMPLEMENTED; };
  };
  struct SamplerStateData : public RendererResource
  {
    AddressMode u = ( AddressMode )0;
    AddressMode v = ( AddressMode )0;
    AddressMode w = ( AddressMode )0;
    Comparison compare = ( Comparison )0;
    Filter filter = ( Filter )0;
  };
  struct SamplerState : public SamplerStateData { };
  struct TextureData : public RendererResource
  {
    virtual void* GetImguiTextureID() { return nullptr; }
    float GetAspect() { return ( float )myImage.mWidth / ( float )myImage.mHeight; }
    Image myImage;
    Access access = Access::Default;
    std::set< CPUAccess > cpuAccess;
    std::set< Binding > binding;
  };
  struct Texture : public TextureData
  {
    virtual void Clear() {}
  };
  struct DepthBufferData : public RendererResource
  {
    int width = 0;
    int height = 0;

    int mDepthBitCount = 0;
    GraphicsType mDepthGraphicsType = GraphicsType::unknown;

    int mStencilBitCount = 0;
    GraphicsType mStencilType = GraphicsType::unknown;
  };
  struct DepthBuffer : public DepthBufferData
  {
    virtual void Clear() {}
  };
  struct CBufferData : public RendererResource
  {
    int shaderRegister = 0;
    int byteCount = 0;
    virtual void SendUniforms( void* bytes ) {}
  };
  struct CBuffer : public CBufferData { };
  struct BlendStateData : public RendererResource
  {
    // TODO: init defaults
    BlendConstants srcRGB;
    BlendConstants dstRGB;
    BlendMode blendRGB;
    BlendConstants srcA;
    BlendConstants dstA;
    BlendMode blendA;
  };
  struct BlendState : public BlendStateData { };
  struct RasterizerStateData : public RendererResource
  {
    FillMode fillMode = ( FillMode )0;
    CullMode cullMode = ( CullMode )0;
    bool frontCounterClockwise = false;
    bool scissor = false;
    bool multisample = false;
  };
  struct RasterizerState : public RasterizerStateData { };
  struct DepthStateData : public RendererResource
  {
    bool depthTest = false;
    bool depthWrite = false;
    DepthFunc depthFunc = ( DepthFunc )0;
  };
  struct DepthState : public DepthStateData { };
  struct VertexFormatData : public RendererResource
  {
    Vector< VertexDeclaration > vertexFormatDatas;
    Shader* shader = nullptr;
  };
  struct VertexFormat : public VertexFormatData { };

  struct DefaultCBufferPerFrame
  {
    m4 mView;
    m4 mProjection;
    float mFar;
    float mNear;
    v2 mGbufferSize;
    static String name_view() { return "View"; };
    static String name_proj() { return "Projection"; };
    static String name_far() { return "far"; };
    static String name_near() { return "near"; };
    static String name_gbuffersize() { return "gbufferSize"; };
    static const int shaderRegister = 0;
  };
  struct DefaultCBufferPerObject
  {
    m4 World;
    v4 Color;
    static String name_world() { return "World"; };
    static String name_color() { return "Color"; };
    static const int shaderRegister = 1;
  };

  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );

  struct ScissorRect
  {
    float mXMinRelUpperLeftCornerPixel = 0;
    float mYMinRelUpperLeftCornerPixel = 0;
    float mXMaxRelUpperLeftCornerPixel = 0;
    float mYMaxRelUpperLeftCornerPixel = 0;
  };

  // glViewport lets opengl know how to map the NDC coordinates to the framebuffer coordinates.
  struct Viewport
  {
    float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerX = 0;
    float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerY = 0;
    float mViewportPixelWidthIncreasingRight = 0;
    float mViewportPixelHeightIncreasingUp = 0;
    float mMinDepth = 0;
    float mMaxDepth = 1;
  };

  struct RenderView
  {
    Texture* mFramebuffer = nullptr;
    DepthBuffer* mFramebufferDepth = nullptr;
    Viewport mViewportRect;
    ScissorRect mScissorRect;
    m4 mView = m4::Identity();
    m4 mProj = m4::Identity();
    v4 mClearColorRGBA = v4( 0, 0, 0, 1 );
  };

  enum PrimitiveTopology
  {
    TriangleList,
    LineList,
    Count,
  };

  struct DrawCall2
  {
    Shader* mShader = nullptr;
    VertexBuffer* mVertexBuffer = nullptr;
    IndexBuffer* mIndexBuffer = nullptr;
    int mStartIndex = 0;
    int mIndexCount = 0;
    int mVertexCount = 0;
    RenderView* mRenderView = nullptr;
    BlendState* mBlendState = nullptr;
    RasterizerState* mRasterizerState = nullptr;
    SamplerState* mSamplerState = nullptr;
    DepthState* mDepthState = nullptr;
    VertexFormat* mVertexFormat = nullptr;
    Vector< const Texture* > mTextures;
    CBuffer* mUniformDst = nullptr;
    Vector< char > mUniformSrcc;
    StackFrame mFrame;
    PrimitiveTopology mPrimitiveTopology = PrimitiveTopology::TriangleList;

    template< typename T>
    void CopyUniformSource( const T& t ) { CopyUniformSource( &t, sizeof( T ) ); }
    void CopyUniformSource( const void* bytes, int byteCount );
  };


  namespace Render
  {
    typedef int ViewId;
    typedef int ResourceId;
    const ResourceId NullResourceId = -1;

    struct VertexBufferHandle { ResourceId mResourceId = NullResourceId; };
    struct IndexBufferHandle { ResourceId mResourceId = NullResourceId; };
    struct TextureHandle { ResourceId mResourceId = NullResourceId; };
    struct FramebufferHandle { ResourceId mResourceId = NullResourceId; };

    void SubmitAllocInit( int ringBufferByteCount );
    void* SubmitAlloc( int byteCount );
    void* SubmitAlloc( void* bytes, int byteCount );
    //void SubmitAllocBeginFrame();

    VertexBufferHandle               CreateVertexBuffer( StringView, StackFrame );
    IndexBufferHandle                CreateIndexBuffer( StringView, StackFrame );
    TextureHandle                    CreateTexture( StringView, StackFrame );
    FramebufferHandle                CreateFramebuffer( void* nativeWindowHandle,
                                                        int width,
                                                        int height,
                                                        StringView,
                                                        StackFrame );

    void                             DestroyVertexBuffer( VertexBufferHandle );
    void                             DestroyIndexBuffer( IndexBufferHandle );
    void                             DestroyTexture( TextureHandle );
    void                             DestroyFramebuffer( FramebufferHandle );

    void                             UpdateTextureRegion( TextureHandle mDst,
                                                          Image mSrc,
                                                          int mDstX,
                                                          int mDstY );
    void                             UpdateVertexBuffer( VertexBufferHandle,
                                                         void* bytes,
                                                         int byteCount );
    void                             UpdateIndexBuffer( IndexBufferHandle,
                                                        void* bytes,
                                                        int byteCount );
  }

  struct Renderer
  {
    enum class Type
    {
      Vulkan,
      OpenGL4,
      DirectX12,
      Count,
    };

    static Renderer* Instance;
    Renderer();
    virtual void CreateWindowContext( DesktopWindow* desktopWindow, Errors& errors ) {}

    virtual ~Renderer();
    virtual void Init( Errors& errors ) {};
    virtual void AddVertexBuffer(
      VertexBuffer** vertexBuffer,
      const VertexBufferData& vertexBufferData,
      Errors& errors ) {
      AddRendererResource( vertexBuffer, vertexBufferData );
    }

    virtual void AddIndexBuffer(
      IndexBuffer** indexBuffer,
      const IndexBufferData& indexBufferData,
      Errors& errors ) {
      AddRendererResource( indexBuffer, indexBufferData );
    }

    virtual void ClearColor( Texture* texture, v4 rgba ) { TAC_UNIMPLEMENTED; }
    virtual void ClearDepthStencil(
      DepthBuffer* depthBuffer,
      bool shouldClearDepth,
      float depth,
      bool shouldClearStencil,
      uint8_t stencil )
    {
      TAC_UNIMPLEMENTED;
    }

    virtual void ReloadShader( Shader* shader, Errors& errors ) { TAC_UNIMPLEMENTED; }
    virtual void AddShader( Shader** shader, const ShaderData& shaderData, Errors& errors ) { AddRendererResource( shader, shaderData ); }
    virtual void GetShaders( Vector< Shader* > & ) { TAC_UNIMPLEMENTED; }

    virtual void AddSamplerState( SamplerState** samplerState, const SamplerStateData& samplerStateData, Errors& errors )
    {
      AddRendererResource( samplerState, samplerStateData );
    }
    virtual void AddSampler(
      const String& samplerName,
      Shader* shader,
      ShaderType shaderType,
      int samplerIndex )
    {
      TAC_UNIMPLEMENTED;
    }
    virtual void SetSamplerState(
      const String& samplerName,
      SamplerState* samplerState )
    {
      TAC_UNIMPLEMENTED;
    }

    virtual void AddTextureResource( Texture** texture, const TextureData& textureData, Errors& errors ) {
      AddRendererResource( texture, textureData );
    }
    virtual void AddTextureResourceCube( Texture** texture, const TextureData& textureData, void** sixCubeDatas, Errors& errors ) {
      AddRendererResource( texture, textureData );
    }
    virtual void AddTexture(
      const String& textureName,
      Shader* shader,
      ShaderType shaderType,
      int samplerIndex )
    {
      TAC_UNIMPLEMENTED;
    }
    //virtual void MapTexture(
    //  Texture* texture,
    //  void** data,
    //  Map mapType,
    //  Errors& errors ) { Unimplemented; }
    //virtual void UnmapTexture( Texture* texture ) { Unimplemented; }

    // x-axis increases right, y-axis increases downwards, ( 0, 0 ) is top left ( directx style )
    // x, y are also the top left corner of src image
    virtual void CopyTextureRegion(
      Texture* dst,
      Image src,
      int x,
      int y,
      Errors& errors )
    {
      TAC_UNIMPLEMENTED;
    }
    virtual void SetTexture(
      const String& textureName,
      Texture* texture )
    {
      TAC_UNIMPLEMENTED;
    }

    virtual void AddDepthBuffer(
      DepthBuffer** outputDepthBuffer,
      const DepthBufferData& depthBufferData,
      Errors& errors )
    {
      AddRendererResource( outputDepthBuffer, depthBufferData );
    }


    template< typename TResource, typename TResourceData >
    void AddRendererResource( TResource** ppResource, const TResourceData& resourceData )
    {
      if( IsDebugMode() )
      {
        auto resource = ( RendererResource* )&resourceData;
        TAC_ASSERT( resource->mName.size() );
        TAC_ASSERT( resource->mFrame.mFile.size() );
        TAC_ASSERT( resource->mFrame.mFunction.size() );
      }

      auto resource = new TResource();
      *( TResourceData* )resource = resourceData;
      resource->mActive = true;

      mRendererResources.insert( resource );

      *ppResource = resource;
    }
    void RemoveRendererResource( RendererResource* rendererResource );

    std::set< RendererResource* > mRendererResources;

    virtual void AddConstantBuffer( CBuffer** outputCbuffer, const CBufferData& cBufferData, Errors& errors )
    {
      AddRendererResource( outputCbuffer, cBufferData );
    }
    virtual void AddCbufferToShader( Shader* shader, CBuffer* cbuffer, ShaderType myShaderType )
    {
      //Unimplemented;
    }

    virtual void AddBlendState( BlendState** blendState, const BlendStateData& blendStateData, Errors& errors )
    {
      AddRendererResource( blendState, blendStateData );
    }

    virtual void AddRasterizerState(
      RasterizerState** rasterizerState,
      const RasterizerStateData& rasterizerStateData,
      Errors& errors )
    {
      AddRendererResource( rasterizerState, rasterizerStateData );
    }

    virtual void AddDepthState( DepthState** depthState, const DepthStateData& depthStateData, Errors& errors )
    {
      AddRendererResource( depthState, depthStateData );
    }


    virtual void AddVertexFormat( VertexFormat** vertexFormat, const VertexFormatData& vertexFormatData, Errors& errors )
    {
      AddRendererResource( vertexFormat, vertexFormatData );
    }

    virtual void DebugBegin( const String& section ) { TAC_UNIMPLEMENTED; }
    virtual void DebugMark( const String& remark ) { TAC_UNIMPLEMENTED; }
    virtual void DebugEnd() { TAC_UNIMPLEMENTED; }

    virtual void DrawNonIndexed( int vertexCount = 0 ) { TAC_UNIMPLEMENTED; }

    virtual void DrawIndexed( int elementCount, int idxOffset, int vtxOffset ) { TAC_UNIMPLEMENTED; }

    virtual void Apply() { TAC_UNIMPLEMENTED; }

    virtual void RenderFlush() { TAC_UNIMPLEMENTED; }
    virtual void Render( Errors& errors ) { TAC_UNIMPLEMENTED; }

    virtual void SetPrimitiveTopology( Primitive primitive ) { TAC_UNIMPLEMENTED; }

    virtual void GetPerspectiveProjectionAB(
      float f,
      float n,
      float& a,
      float& b ) {
      TAC_UNIMPLEMENTED;
    }

    //void DebugDraw(
    //  m4 world_to_view,
    //  m4 view_to_clip,
    //  Texture* texture,
    //  DepthBuffer* depth,
    //  const Vector< DefaultVertexColor >& mDebugDrawVerts,
    //  Errors& errors );
    void GetUIDimensions( Texture* texture, float* width, float* height );
    void DebugImgui();


    String mName;

    // Should this resize and return a reference to theback?
    void AddDrawCall( const DrawCall2& drawCall )
    {
      mDrawCall2s.push_back( drawCall );
    }
    Vector< DrawCall2 > mDrawCall2s;
  };

  struct RendererFactory
  {
  public:
    virtual ~RendererFactory() = default;
    void CreateRendererOuter();
    String mRendererName;
  protected:
    virtual void CreateRenderer() { TAC_UNIMPLEMENTED; }
  };

  struct RendererRegistry
  {
    static RendererRegistry& Instance();
    RendererFactory* FindFactory( StringView name );

    Vector< RendererFactory* > mFactories;
  };

  String ToString( Renderer::Type );
  const static String RendererNameVulkan = "Vulkan";
  const static String RendererNameOpenGL4 = "OpenGL4";
  const static String RendererNameDirectX11 = "DirectX11";
  const static String RendererNameDirectX12 = "DirectX12";
}
