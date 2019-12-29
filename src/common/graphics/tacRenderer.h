// renderer interface
// used for creating the things necessary to put pretty pixels on the screen
// ( textures, shaders, geometry ( vertexes + indexes ) )

#pragma once

#include "common/containers/tacVector.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector3.h"
#include "common/math/tacVector4.h"
#include "common/math/tacMatrix4.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacPreprocessor.h"
//#include "common/tacSpan.h"

#include <set>

struct TacDesktopWindow;
struct TacRenderer;
struct TacCBuffer;
struct TacShell;

const v4 colorGrey = v4( v3( 1, 1, 1 ) * 95.0f, 255 ) / 255.0f;
const v4 colorOrange = v4( 255, 200, 84, 255 ) / 255.0f;
const v4 colorGreen = v4( 0, 255, 112, 255 ) / 255.0f;
const v4 colorBlue = v4( 84, 255, 255, 255 ) / 255.0f;
const v4 colorRed = v4( 255, 84, 84, 255 ) / 255.0f;
const v4 colorMagenta = v4( 255, 84, 255, 255 ) / 255.0f;


enum class TacRendererType
{
  Vulkan,
  OpenGL4,
  DirectX12,
  Count,
};
TacString TacToString( TacRendererType rendererType );

const TacString RendererNameVulkan = "Vulkan";
const TacString RendererNameOpenGL4 = "OpenGL4";
const TacString RendererNameDirectX11 = "DirectX11";
const TacString RendererNameDirectX12 = "DirectX12";


enum class TacAttribute // Used to hardcode shader semantics/indexes
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
enum class TacGraphicsType
{
  unknown,
  sint,
  uint,
  snorm,
  unorm,
  real
};
enum class TacDepthFunc
{
  Less,
  LessOrEqual,
};
enum class TacAddressMode
{
  Wrap,
  Clamp,
  Border,
};
enum class TacComparison
{
  Always,
  Never,
};
enum class TacFilter
{
  Point,
  Linear
};
enum class TacShaderType
{
  Vertex,
  Fragment,
  Count,
};
enum class TacAccess
{
  Static, // Never gonna change
  Default, // ?
  Dynamic, // Gonna change ( debug draw, animation )
};
enum class TacCPUAccess
{
  Read,
  Write
};
enum class TacMap
{
  Read,
  Write,
  ReadWrite,
  WriteDiscard, // the previous contents will be discarded
};
enum class TacPrimitive
{
  TriangleList,
  LineList
};
enum class TacBlendMode
{
  Add,
};
enum class TacBlendConstants
{
  One,
  Zero,
  SrcRGB,
  SrcA,
  OneMinusSrcA,
};
enum class TacFillMode
{
  Solid,
  Wireframe
};
enum class TacCullMode
{
  None,
  Back,
  Front
};
enum class TacBinding
{
  ShaderResource,
  RenderTarget,
};

const char* TacGetSemanticName( TacAttribute attribType );

// Used so the gpu can translate from cpu types to gpu types
struct TacFormat
{
  int CalculateTotalByteCount() const;
  int mElementCount = 0;
  int mPerElementByteCount = 0;
  TacGraphicsType mPerElementDataType = TacGraphicsType::unknown;
};

const TacFormat formatv2 = { 2, sizeof( float ), TacGraphicsType::real };
const TacFormat formatv3 = { 3, sizeof( float ), TacGraphicsType::real };


struct TacImage
{
  int mWidth = 0;
  int mHeight = 0;

  // comment?
  int mPitch = 0;
  void* mData = nullptr;
  TacFormat mFormat;
};
struct TacConstant
{
  TacString mName;
  int mOffset = 0;
  int mSize = 0;
};
struct TacVertexDeclaration
{
  TacAttribute mAttribute = TacAttribute::Count;
  TacFormat mTextureFormat;

  // Offset of the variable from the vertex buffer
  // ie: TacOffsetOf( MyVertexType, mPosition)
  int mAlignedByteOffset = 0;
};




// don't store these in a TacOwned, they should be both new'd and delete'd by the renderer
struct TacRendererResource
{
  virtual ~TacRendererResource()
  {
    // Please call remove renderer resource instead of deleting directly
    TacAssert( mActive == false );
  }
  bool mActive = false;
  TacString mName;
  TacStackFrame mStackFrame;
};

struct TacShaderData : public TacRendererResource
{
  // can load from either
  TacString mShaderPath;
  TacString mShaderStr;

  TacVector< TacCBuffer* > mCBuffers;
};
struct TacShader : public TacShaderData { };// for now, this encompasses both the vertex & pixel shader 
struct TacVertexBufferData : public TacRendererResource
{
  TacAccess mAccess = TacAccess::Default;
  void* mOptionalData = nullptr;
  int mNumVertexes = 0;
  int mStrideBytesBetweenVertexes = 0;
};
struct TacVertexBuffer : public TacVertexBufferData
{
  virtual void Overwrite( void* data, int byteCount, TacErrors& errors ) { TacUnimplemented; };
};
struct TacIndexBufferData : public TacRendererResource
{
  TacAccess mAccess = TacAccess::Default;
  const void* mData = nullptr;
  int mIndexCount = 0;
  TacFormat mFormat;
};
struct TacIndexBuffer : public TacIndexBufferData
{
  virtual void Overwrite( void* data, int byteCount, TacErrors& errors ) { TacUnimplemented; };
};
struct TacSamplerStateData : public TacRendererResource
{
  TacAddressMode u = (TacAddressMode)0;
  TacAddressMode v = (TacAddressMode)0;
  TacAddressMode w = (TacAddressMode)0;
  TacComparison compare = (TacComparison)0;
  TacFilter filter = (TacFilter)0;
};
struct TacSamplerState : public TacSamplerStateData { };
struct TacTextureData : public TacRendererResource
{
  virtual void* GetImguiTextureID() { return nullptr; }
  float GetAspect() { return ( float )myImage.mWidth / ( float )myImage.mHeight; }
  TacImage myImage;
  TacAccess access = TacAccess::Default;
  std::set< TacCPUAccess > cpuAccess;
  std::set< TacBinding > binding;
};
struct TacTexture : public TacTextureData 
{ 
  virtual void Clear() {}
};
struct TacDepthBufferData : public TacRendererResource
{
  int width = 0;
  int height = 0;

  int mDepthBitCount = 0;
  TacGraphicsType mDepthGraphicsType = TacGraphicsType::unknown;

  int mStencilBitCount = 0;
  TacGraphicsType mStencilType = TacGraphicsType::unknown;
};
struct TacDepthBuffer : public TacDepthBufferData
{
  virtual void Clear() {}
};
struct TacCBufferData : public TacRendererResource
{
  int shaderRegister = 0;
  int byteCount = 0;
  virtual void SendUniforms( void* bytes ) {}
};
struct TacCBuffer : public TacCBufferData { };
struct TacBlendStateData : public TacRendererResource
{
  // TODO: init defaults
  TacBlendConstants srcRGB;
  TacBlendConstants dstRGB;
  TacBlendMode blendRGB;
  TacBlendConstants srcA;
  TacBlendConstants dstA;
  TacBlendMode blendA;
};
struct TacBlendState : public TacBlendStateData { };
struct TacRasterizerStateData : public TacRendererResource
{
  TacFillMode fillMode = (TacFillMode)0;
  TacCullMode cullMode = (TacCullMode)0;
  bool frontCounterClockwise = false;
  bool scissor = false;
  bool multisample = false;
};
struct TacRasterizerState : public TacRasterizerStateData { };
struct TacDepthStateData : public TacRendererResource
{
  bool depthTest = false;
  bool depthWrite = false;
  TacDepthFunc depthFunc = (TacDepthFunc)0;
};
struct TacDepthState : public TacDepthStateData { };
struct TacVertexFormatData : public TacRendererResource
{
  TacVector< TacVertexDeclaration > vertexFormatDatas;
  TacShader* shader = nullptr;
};
struct TacVertexFormat : public TacVertexFormatData { };

struct TacDefaultCBufferPerFrame
{
  m4 mView;
  m4 mProjection;
  float mFar;
  float mNear;
  v2 mGbufferSize;
  static TacString name_view() { return "View"; };
  static TacString name_proj() { return "Projection"; };
  static TacString name_far() { return "far"; };
  static TacString name_near() { return "near"; };
  static TacString name_gbuffersize() { return "gbufferSize"; };
  static const int shaderRegister = 0;
};
struct TacDefaultCBufferPerObject
{
  m4 World;
  v4 Color;
  static TacString name_world() { return "World"; };
  static TacString name_color() { return "Color"; };
  static const int shaderRegister = 1;
};

v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );

struct TacScissorRect
{
  float mXMinRelUpperLeftCornerPixel = 0;
  float mYMinRelUpperLeftCornerPixel = 0;
  float mXMaxRelUpperLeftCornerPixel = 0;
  float mYMaxRelUpperLeftCornerPixel = 0;
};

// glViewport lets opengl know how to map the NDC coordinates to the framebuffer coordinates.
struct TacViewport
{
  float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerX = 0;
  float mViewportBottomLeftCornerRelFramebufferBottomLeftCornerY = 0;
  float mViewportPixelWidthIncreasingRight = 0;
  float mViewportPixelHeightIncreasingUp = 0;
  float mMinDepth = 0;
  float mMaxDepth = 1;
};

struct TacRenderView
{
  TacTexture* mFramebuffer = nullptr;
  TacDepthBuffer* mFramebufferDepth = nullptr;
  TacViewport mViewportRect;
  TacScissorRect mScissorRect;
  m4 mView = m4::Identity();
  m4 mProj = m4::Identity();
  v4 mClearColorRGBA = v4( 0, 0, 0, 1 );
};

enum TacPrimitiveTopology
{
  TriangleList,
  LineList,
  Count,
};

struct TacDrawCall2
{
  TacShader* mShader = nullptr;
  TacVertexBuffer* mVertexBuffer = nullptr;
  TacIndexBuffer* mIndexBuffer = nullptr;
  int mStartIndex = 0;
  int mIndexCount = 0;
  int mVertexCount = 0;
  TacRenderView* mRenderView = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacVertexFormat* mVertexFormat = nullptr;
  TacVector< const TacTexture* > mTextures;
  TacCBuffer* mUniformDst = nullptr;
  TacVector< char > mUniformSrcc;
  TacStackFrame mStackFrame;
  TacPrimitiveTopology mPrimitiveTopology = TacPrimitiveTopology::TriangleList;
};

// TODO: Make all the datas passed by const ref

struct TacRenderer
{
  static TacRenderer* Instance;
  TacRenderer();
  virtual void CreateWindowContext( TacDesktopWindow* desktopWindow, TacErrors& errors ) {}

  virtual ~TacRenderer();
  virtual void Init( TacErrors& errors ) {};
  virtual void AddVertexBuffer(
    TacVertexBuffer** vertexBuffer,
    const TacVertexBufferData& vertexBufferData,
    TacErrors& errors ) {
    AddRendererResource( vertexBuffer, vertexBufferData );
  }

  virtual void AddIndexBuffer(
    TacIndexBuffer** indexBuffer,
    const TacIndexBufferData& indexBufferData,
    TacErrors& errors ) {
    AddRendererResource( indexBuffer, indexBufferData );
  }

  virtual void ClearColor( TacTexture* texture, v4 rgba ) { TacUnimplemented; }
  virtual void ClearDepthStencil(
    TacDepthBuffer* depthBuffer,
    bool shouldClearDepth,
    float depth,
    bool shouldClearStencil,
    uint8_t stencil )
  {
    TacUnimplemented;
  }

  virtual void ReloadShader( TacShader* shader, TacErrors& errors ) { TacUnimplemented; }
  virtual void AddShader( TacShader** shader, const TacShaderData& shaderData, TacErrors& errors ) { AddRendererResource( shader, shaderData ); }
  virtual void GetShaders( TacVector< TacShader* > & ) { TacUnimplemented; }

  virtual void AddSamplerState( TacSamplerState** samplerState, const TacSamplerStateData& samplerStateData, TacErrors& errors )
  {
    AddRendererResource( samplerState, samplerStateData );
  }
  virtual void AddSampler(
    const TacString& samplerName,
    TacShader* shader,
    TacShaderType shaderType,
    int samplerIndex )
  {
    TacUnimplemented;
  }
  virtual void SetSamplerState(
    const TacString& samplerName,
    TacSamplerState* samplerState )
  {
    TacUnimplemented;
  }

  virtual void AddTextureResource( TacTexture** texture, const TacTextureData& textureData, TacErrors& errors ) {
    AddRendererResource( texture, textureData );
  }
  virtual void AddTextureResourceCube( TacTexture** texture, const TacTextureData& textureData, void** sixCubeDatas, TacErrors& errors ) {
    AddRendererResource( texture, textureData );
  }
  virtual void AddTexture(
    const TacString& textureName,
    TacShader* shader,
    TacShaderType shaderType,
    int samplerIndex )
  {
    TacUnimplemented;
  }
  //virtual void MapTexture(
  //  TacTexture* texture,
  //  void** data,
  //  TacMap mapType,
  //  TacErrors& errors ) { TacUnimplemented; }
  //virtual void UnmapTexture( TacTexture* texture ) { TacUnimplemented; }

  // x-axis increases right, y-axis increases downwards, ( 0, 0 ) is top left ( directx style )
  // x, y are also the top left corner of src image
  virtual void CopyTextureRegion(
    TacTexture* dst,
    TacImage src,
    int x,
    int y,
    TacErrors& errors )
  {
    TacUnimplemented;
  }
  virtual void SetTexture(
    const TacString& textureName,
    TacTexture* texture )
  {
    TacUnimplemented;
  }

  virtual void AddDepthBuffer(
    TacDepthBuffer** outputDepthBuffer,
    const TacDepthBufferData& depthBufferData,
    TacErrors& errors )
  {
    AddRendererResource( outputDepthBuffer, depthBufferData );
  }


  template< typename TResource, typename TResourceData >
  void AddRendererResource( TResource** ppResource, const TResourceData& resourceData )
  {
    if( TacIsDebugMode() )
    {
      auto resource = ( TacRendererResource* )&resourceData;
      TacAssert( resource->mName.size() );
      TacAssert( resource->mStackFrame.mFile.size() );
      TacAssert( resource->mStackFrame.mFunction.size() );
    }

    auto resource = new TResource();
    *( TResourceData* )resource = resourceData;
    resource->mActive = true;

    mRendererResources.insert( resource );

    *ppResource = resource;
  }
  void RemoveRendererResource( TacRendererResource* rendererResource );

  std::set< TacRendererResource* > mRendererResources;

  virtual void AddConstantBuffer( TacCBuffer** outputCbuffer, const TacCBufferData& cBufferData, TacErrors& errors )
  {
    AddRendererResource( outputCbuffer, cBufferData );
  }
  virtual void AddCbufferToShader( TacShader* shader, TacCBuffer* cbuffer, TacShaderType myShaderType )
  {
    //TacUnimplemented;
  }

  virtual void AddBlendState( TacBlendState** blendState, const TacBlendStateData& blendStateData, TacErrors& errors )
  {
    AddRendererResource( blendState, blendStateData );
  }

  virtual void AddRasterizerState(
    TacRasterizerState** rasterizerState,
    const TacRasterizerStateData& rasterizerStateData,
    TacErrors& errors )
  {
    AddRendererResource( rasterizerState, rasterizerStateData );
  }

  virtual void AddDepthState( TacDepthState** depthState, const TacDepthStateData& depthStateData, TacErrors& errors )
  {
    AddRendererResource( depthState, depthStateData );
  }


  virtual void AddVertexFormat( TacVertexFormat** vertexFormat, const TacVertexFormatData& vertexFormatData, TacErrors& errors )
  {
    AddRendererResource( vertexFormat, vertexFormatData );
  }

  virtual void DebugBegin( const TacString& section ) { TacUnimplemented; }
  virtual void DebugMark( const TacString& remark ) { TacUnimplemented; }
  virtual void DebugEnd() { TacUnimplemented; }

  virtual void DrawNonIndexed( int vertexCount = 0 ) { TacUnimplemented; }

  virtual void DrawIndexed( int elementCount, int idxOffset, int vtxOffset ) { TacUnimplemented; }

  virtual void Apply() { TacUnimplemented; }

  virtual void RenderFlush() { TacUnimplemented; }
  virtual void Render( TacErrors& errors ) { TacUnimplemented; }

  virtual void SetPrimitiveTopology( TacPrimitive primitive ) { TacUnimplemented; }

  virtual void GetPerspectiveProjectionAB(
    float f,
    float n,
    float& a,
    float& b ) {
    TacUnimplemented;
  }

  //void DebugDraw(
  //  m4 world_to_view,
  //  m4 view_to_clip,
  //  TacTexture* texture,
  //  TacDepthBuffer* depth,
  //  const TacVector< TacDefaultVertexColor >& mDebugDrawVerts,
  //  TacErrors& errors );
  void GetUIDimensions( TacTexture* texture, float* width, float* height );
  void DebugImgui();


  TacShell* mShell = nullptr;
  TacString mName;

  // Should this resize and return a reference to theback?
  void AddDrawCall( const TacDrawCall2& drawCall )
  {
    mDrawCall2s.push_back( drawCall );
  }
  TacVector< TacDrawCall2 > mDrawCall2s;
};

struct TacRendererFactory
{
public:
  virtual ~TacRendererFactory() = default;
  void CreateRendererOuter( TacRenderer** );
  TacString mRendererName;
protected:
  virtual void CreateRenderer( TacRenderer** ) { TacUnimplemented; }
};

struct TacRendererRegistry
{
  static TacRendererRegistry& Instance();
  TacVector< TacRendererFactory* > mFactories;
};
