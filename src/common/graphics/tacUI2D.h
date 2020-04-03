// Draws 2d shit to a texture ( such as the screen )
// like shapes and text and images and shit.

#pragma once

#include "common/tacMemory.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector4.h"
#include "common/math/tacMatrix3.h"
#include "common/tacLocalization.h"
//#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacFont.h"
#include "common/tacErrorHandling.h"

struct TacBlendState;
struct TacCBuffer;
struct TacDepthState;
struct TacFontStuff;
struct TacIndexBuffer;
struct TacRasterizerState;
struct TacRenderView;
struct TacSamplerState;
struct TacShader;
struct TacUI2DCommonData;
struct TacUI2DDrawCall;
struct TacUI2DDrawData;
struct TacUI2DState;
struct TacVertexBuffer;
struct TacVertexFormat;

typedef uint16_t TacUI2DIndex; // formerly TacDefaultIndex2D

struct TacUI2DVertex
{
  v2 mPosition;
  v2 mGLTexCoord;
};

struct TacUI2DCommonData
{
  static TacUI2DCommonData* Instance;
  TacUI2DCommonData();
  ~TacUI2DCommonData();
  void Init( TacErrors& errors );

  TacTexture* m1x1White = nullptr;
  TacFontStuff* mFontStuff = nullptr;
  TacVertexFormat* mFormat = nullptr;
  TacShader* mShader = nullptr;
  TacShader* m2DTextShader = nullptr;
  TacDepthState* mDepthState = nullptr;
  TacBlendState* mBlendState = nullptr;
  TacRasterizerState* mRasterizerState = nullptr;
  TacSamplerState* mSamplerState = nullptr;
  TacCBuffer* mPerFrame = nullptr;
  TacCBuffer* mPerObj = nullptr;
};

struct TacUI2DState
{
  void Translate( v2 pos );
  void Translate( float x, float y );
  void Draw2DBox(
    float width,
    float height,
    v4 color = { 1, 1, 1, 1 },
    TacTexture* texture = nullptr );

  void Draw2DText(
    TacLanguage mDefaultLanguage,
    int fontSize,
    const TacString& text,
    float* heightBetweenBaselines,
    v4 color,
    TacErrors& errors );

  m3 mTransform = m3::Identity();

  TacUI2DDrawData* mUI2DDrawData = nullptr;
};

struct TacUI2DDrawCall
{
  int mIVertexStart = 0;
  int mVertexCount = 0;
  int mIIndexStart = 0;
  int mIndexCount = 0;

  TacShader* mShader = nullptr;
  const TacTexture* mTexture = nullptr;
  TacVector< char > mUniformSource;

  void CopyUniform( const void* bytes, int byteCount );

  template< typename T>
  void CopyUniform( T& t ) { CopyUniform( &t, sizeof(T)); }
};

struct TacImGuiRect;

// why does this class exist
struct TacUI2DDrawData
{
  TacUI2DDrawData();
  ~TacUI2DDrawData();
  void DrawToTexture( TacErrors& errors );

  TacUI2DState* PushState();
  void PopState();

  v2 CalculateTextSize( const TacString& text, int fontSize );
  v2 CalculateTextSize( const TacVector< TacCodepoint >& codepoints, int fontSize );
  v2 CalculateTextSize( const TacCodepoint* codepoints, int codepointCount, int fontSize );
  void AddText( v2 textPos, int fontSize, const TacString& utf8, v4 color, const TacImGuiRect* clipRect );
  void AddBox( v2 mini, v2 maxi, v4 color, const TacTexture* texture, const TacImGuiRect* clipRect );
  void AddLine( v2 p0, v2 p1, float radius, v4 color );
  //void AddPolyFill( const TacVector< v2 >& points, v4 color );

  TacVector< TacUI2DVertex > mDefaultVertex2Ds;
  TacVector< TacUI2DIndex > mDefaultIndex2Ds;
  TacVector< TacUI2DDrawCall > mDrawCall2Ds;
  TacVector< TacUI2DState > mStates;

  TacVertexBuffer* mVerts = nullptr;
  TacIndexBuffer* mIndexes = nullptr;
  TacRenderView* mRenderView = nullptr;
};

