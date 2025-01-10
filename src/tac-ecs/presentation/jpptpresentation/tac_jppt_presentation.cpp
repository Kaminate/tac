#include "tac_jppt_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac-ecs/presentation/jpptpresentation/tac_jppt_BVH.h"


#if TAC_JPPT_PRESENTATION_ENABLED()

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  static bool                 sInitialized;
  static bool                 sEnabled;
  static Render::BufferHandle sCameraGPUBuffer;
  static SceneBVH*            sSceneBvh;
  static const World*         sWorld;
  static const Camera*        sCamera;

  struct JPPTCamera
  {
    m4    mFrame        { 1 };
    float mLens         { 0.05f }; // ?
    float mFilm         { 0.036f }; // ?
    float mAspect       { 1.5f };
    float mFocus        { 1000 }; // ?
    v3    mPad0         {};
    float mAperture     {};
    i32   mOrthographic {};
    v3    mPad1         {};
  };
  // ^ wat is with this weird padding


  //struct JPPTVertex
  //{
  //  v3 mPos;
  //  v2 mTexCoord;
  //  v4 mColor;
  //};

  void             JPPTPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;


    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::CreateBufferParams bufferParams
    {
      .mByteCount     { sizeof( JPPTCamera ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ShaderResource },
      .mOptionalName  { "jpptcamera" },
    };
    TAC_CALL( sCameraGPUBuffer = renderDevice->CreateBuffer( bufferParams, errors ) );



    sInitialized = true;
  }

  void             JPPTPresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void             JPPTPresentation::Render( Render::IContext* renderContext,
                                             const World* world,
                                             const Camera* camera,
                                             const v2i viewSize,
                                             const Render::TextureHandle dstColorTex,
                                             const Render::TextureHandle dstDepthTex,
                                             Errors& errors )
  {
    TAC_ASSERT( sInitialized );
    if( !sEnabled )
      return;

    sWorld = world;
    sCamera = camera;
  }

  void             JPPTPresentation::DebugImGui()
  {
    if( !ImGuiCollapsingHeader( "JPPT" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "JPPT Presentation Enabled", &sEnabled );
    if( !sEnabled )
      return;

    static Errors createBVHErrors;
    if( ImGuiButton( "Create Scene BVH" ) )
      sSceneBvh = SceneBVH::CreateBVH( sWorld, createBVHErrors );

    if( createBVHErrors )
    {
      ImGuiText( createBVHErrors.ToString() );
    }
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac

#endif // TAC_JPPT_PRESENTATION_ENABLED
