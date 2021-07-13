#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/math/tacMath.h"
#include "src/space/light/tacLight.h"

namespace Tac
{
  static struct LightTypeNames
  {
    friend static const char* LightTypeToName( Light::Type );
    //friend static Light::Type LightTypeFromName( const char* );
    LightTypeNames()
    {
      mNames[ Light::Type::kSpot ] = "Spot";
      mNames[ Light::Type::kDirectional ] = "Directional";
      for( int i = 0; i < Light::Type::kCount; ++i )
        TAC_ASSERT( mNames[ i ] );
    }
  private:

    const char* GetName( const Light::Type type )
    {
      return mNames[ type ];
    }

    //Light::Type GetType( const char* name )
    //{
    //  for( int i = 0; i < Light::Type::kCount; ++i )
    //    if( !StrCmp( mNames[ i ], name ) )
    //      return ( Light::Type )i;
    //  return Light::Type::kCount;
    //}

    const char* mNames[ Light::Type::kCount ] = {};
  } sLightTypeNames;

  static const char* LightTypeToName( const Light::Type type ) { return sLightTypeNames.GetName( type ); }
  //static Light::Type LightTypeFromName( const char* name )     { return sLightTypeNames.GetType( name ); }


  static void LightDebugImguiType( Light* light )
  {
    ImGuiText( FrameMemoryPrintf( "Light type: %s", LightTypeToName( light->mType ) ) );
    ImGuiText( "Change light type: " );
    for( Light::Type type = ( Light::Type )0; type < Light::Type::kCount; type = ( Light::Type )( type + 1 ) )
    {
      if( type == light->mType )
        continue;
      ImGuiSameLine();
      const char* name = LightTypeToName( type );
      if( ImGuiButton( name ) )
        light->mType = type;
    }

    if( light->mType == Light::kSpot
        && ImGuiCollapsingHeader( FrameMemoryPrintf( "%s light parameters", LightTypeToName( light->mType ) ) ) )
    {
      float fovDeg = light->mSpotHalfFOVRadians * ( 180.0f / 3.14f );
      if( ImGuiDragFloat( "half fov deg", &fovDeg ) )
      {
        const float eps = 1.0f;
        fovDeg = Max( fovDeg, eps );
        fovDeg = Min( fovDeg, 90.0f - eps );
        light->mSpotHalfFOVRadians = fovDeg * ( 3.14f / 180.0f );
      }
    }

  }

  void LightDebugImguiShadowResolution( Light* light )
  {
    int newDim = light->mShadowResolution;
    if( ImGuiButton( "-" ) )
      newDim /= 2;
    ImGuiSameLine();
    if( ImGuiButton( "+" ) )
      newDim *= 2;
    ImGuiSameLine();
    newDim = Max( newDim, 64 );
    newDim = Min( newDim, 1024 );
    ImGuiText( FrameMemoryPrintf( "Shadow Resolution %ix%i", light->mShadowResolution ) );
  }

  void LightDebugImgui( Light* light )
  {
    const int oldShadowMapResolution = light->mShadowResolution;
    LightDebugImguiType( light );
    ImGuiCheckbox( "Casts shadows", &light->mCastsShadows );
    //ImGuiImage( ( int )light->mShadowMapColor, { 100, 100 } );
    LightDebugImguiShadowResolution( light );
    ImGuiImage( ( int )light->mShadowMapDepth, { 100, 100 } );

    if( light->mShadowResolution != oldShadowMapResolution )
    {
     light->FreeRenderResources();
    }
  }
}

