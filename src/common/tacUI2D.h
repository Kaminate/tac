// Draws 2d shit to a texture ( such as the screen )
// like shapes and text and images and shit.

#pragma once

#include "common/tacMemory.h"
#include "common/math/tacVector2.h"
#include "common/math/tacMatrix3.h"
#include "common/tacLocalization.h"
#include "common/tacRenderer.h"
#include "common/tacErrorHandling.h"
#include "common/tacFont.h"

struct TacUI2DCommonData;
struct TacUI2DDrawCall;
struct TacUI2DDrawData;
struct TacUI2DState;
struct TacUI2DVertex;
struct TacFontStuff;

typedef uint16_t TacUI2DIndex;

struct TacUI2DVertex
{
  v2 mPosition;
  v2 mGLTexCoord;
  static TacVertexDeclarations sVertexDeclarations;
};

struct TacUI2DCommonData
{
  //TacUI2DCommonData();
  TacUI2DCommonData() = default;
  ~TacUI2DCommonData();
  void Init( TacErrors& errors );

  TacTexture* m1x1White = nullptr;
  TacFontStuff* mFontStuff = nullptr;
  TacVertexFormat* mFormat = nullptr;
  TacShader* mShader = nullptr;
  TacShader* m2DTextShader = nullptr;
  TacRenderer* mRenderer = nullptr;
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

  // Shouldn't this be called mVertexCount?
  int mIVertexCount = 0;

  int mIIndexStart = 0;

  // Shouldn't this be called mIndexCount?
  int mIIndexCount = 0;

  TacShader* mShader = nullptr;
  TacTexture* mTexture = nullptr;
  TacVector< char > mUniformSource;
};

struct TacImGuiRect;

// why does this class exist
struct TacUI2DDrawData
{
  ~TacUI2DDrawData();
  void DrawToTexture( TacErrors& errors );

  TacUI2DState* PushState();
  void PopState();

  //TacDrawCall* GetLastDrawCall();

  v2 CalculateTextSize( const TacString& text );
  void AddText( v2 textPos, const TacString& utf8, const TacImGuiRect& clipRect );
  void AddBox( v2 mini, v2 maxi, v4 color, const TacImGuiRect& clipRect, TacTexture* texture = nullptr );
  //void AddPolyFill( const TacVector< v2 >& points, v4 color );

  TacVector< TacDefaultVertex2D > mDefaultVertex2Ds;
  TacVector< TacDefaultIndex2D > mDefaultIndex2Ds;
  TacVector< TacUI2DDrawCall > mDrawCall2Ds;
  TacVector< TacUI2DState > mStates;

  TacVertexBuffer* mVerts = nullptr;
  TacIndexBuffer* mIndexes = nullptr;
  TacRenderView* mRenderView = nullptr;

  TacUI2DCommonData* mUI2DCommonData = nullptr;
};

