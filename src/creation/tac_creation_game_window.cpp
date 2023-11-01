#include "src/common/assetmanagers/tac_mesh.h"
#include "src/common/assetmanagers/tac_model_asset_manager.h"
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/math/tac_math.h"
#include "src/common/math/tac_matrix3.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/creation/tac_creation.h"
#include "src/creation/tac_creation_game_window.h"
#include "src/creation/tac_creation_prefab.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/space/graphics/tac_graphics.h"
#include "src/space/light/tac_light.h"
#include "src/space/model/tac_model.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/presentation/tac_skybox_presentation.h"
#include "src/space/presentation/tac_voxel_gi_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_ghost.h"
#include "src/space/tac_world.h"

#include <cmath>

namespace Tac
{
  static bool drawGrid = false;
  static bool sGizmosEnabled = true;
  static float sWASDCameraPanSpeed = 10;
  static float sWASDCameraOrbitSpeed = 0.1f;
  static bool sWASDCameraOrbitSnap;

  float lightWidgetSize = 6.0f;

  static Render::ShaderHandle spriteShader;

  enum class PickedObject
  {
    None = 0,
    Entity,
    WidgetTranslationArrow,
  };

  struct PickData
  {
    PickedObject pickedObject;
    float        closestDist;
    Entity*      closest;
    int          arrowAxis;
    bool IsNewClosest( float dist ) { return pickedObject == PickedObject::None || dist < closestDist; }
  };
  static PickData pickData;


  struct GameWindowVertex
  {
    v3 pos;
    v3 nor;
  };


  // http://palitri.com/vault/stuff/maths/Rays%20closest%20point.pdf
  //
  //              \ /
  //               \
  //              / \
  //             /   \
  //            /     \    
  //           /   z__\.D 
  //         E.__---  / \
  //         /           \
  //        /             \
  //       /               \
  //     _/                 \_
  //     /| b               |\ a
  //    /                     \
  //  B. <-------- c ----------. A
  //
  //     A - first ray origin
  //     a - first ray direction ( not necessarily normalized )
  //     B - second ray origin
  //     b - second ray direction ( not necessarily normalized )
  //     c - vector from A to B ( c = B - A )
  //     E - closest point on the first ray to the second ray
  //     D - closest point on the second ray to the first ray
  //
  // +-------------+
  // | 3 Equations |
  // +-------------+
  // |
  // +--> Dot( a, z ) = 0
  // +--> Dot( b, z ) = 0
  // +--> c + (b*e) + z - (a*d) = 0
  //
  // +------------+
  // | 3 unknowns |
  // +------------+
  // |
  // +--> e - scalar such that b*e=E
  // +--> d - scalar such that a*d=D
  // +--> z - vector perpendicular to both a and b ( z = D - E )
  static void ClosestPointTwoRays( const v3 A,
                                   const v3 a,
                                   const v3 B,
                                   const v3 b,
                                   float* d,
                                   float* e )
  {
    const v3 c = B - A;
    const float ab = Dot( a, b );
    const float bc = Dot( b, c );
    const float ac = Dot( a, c );
    const float aa = Dot( a, a );
    const float bb = Dot( b, b );
    const float denom = aa * bb - ab * ab;
    if( d )
    {
      *d = ( -ab * bc + ac * bb ) / denom;
    }
    if( e )
    {
      *e = ( ab * ac - bc * aa ) / denom;
    }
  }

  static DefaultCBufferPerFrame GetPerFrame( float w, float h )
  {
    const Camera* camera = gCreation.mEditorCamera;
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane, camera->mNearPlane, a, b );
    return
    {
      .mView = camera->View(),
      .mProjection = m4::ProjPerspective( a, b, camera->mFovyrad, w / h ),
      .mFar = camera->mFarPlane,
      .mNear = camera->mNearPlane,
      .mGbufferSize = { w, h }
    };
  }

  static v3 SnapToUnitDir( const v3 v ) // Returns the unit vector that best aligns with v
  {
    float biggestDot = 0;
    v3 biggestUnitDir = {};
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      for( float sign : { -1.0f, 1.0f } )
      {
        v3 unitDir = {};
        unitDir[ iAxis ] = sign;
        const float d = Dot( v, unitDir );
        if( d > biggestDot )
        {
          biggestDot = d;
          biggestUnitDir = unitDir;
        }
      }
    }
    return biggestUnitDir;
  }

  static void AddDrawCall( const Mesh* mesh, Render::ViewHandle viewHandle )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      Render::SetShader( CreationGameWindow::Instance->m3DShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( CreationGameWindow::Instance->mBlendState );
      Render::SetDepthState( CreationGameWindow::Instance->mDepthState );
      Render::SetRasterizerState( CreationGameWindow::Instance->mRasterizerState );
      Render::SetVertexFormat( CreationGameWindow::Instance->m3DVertexFormat );
      Render::SetSamplerState( CreationGameWindow::Instance->mSamplerState );
      Render::Submit( viewHandle, TAC_STACK_FRAME );
    }
  }


  CreationGameWindow* CreationGameWindow::Instance = nullptr;

  CreationGameWindow::CreationGameWindow()
  {
    Instance = this;
  }

  CreationGameWindow::~CreationGameWindow()
  {
    Instance = nullptr;
    Render::DestroyShader( m3DShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( m3DVertexFormat, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
    DesktopAppDestroyWindow( mDesktopWindowHandle );
    delete mDebug3DDrawData;
  }

  void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
  {


    m3DShader = Render::CreateShader(  "3DTest" , TAC_STACK_FRAME );

    m3DvertexFormatDecls = Render::VertexDeclarations
    {
      {
        .mAttribute = Render::Attribute::Position,
        .mTextureFormat{.mElementCount = 3,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real },
        .mAlignedByteOffset = TAC_OFFSET_OF( GameWindowVertex, pos ),
      },
      {
        .mAttribute = Render::Attribute::Normal,
        .mTextureFormat{ .mElementCount = 3,
                       .mPerElementByteCount = sizeof( float ),
                       .mPerElementDataType = Render::GraphicsType::real},
        .mAlignedByteOffset = TAC_OFFSET_OF( GameWindowVertex, nor ),
      }
    };
    m3DVertexFormat = Render::CreateVertexFormat( m3DvertexFormatDecls,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m3DVertexFormat, "game-window-vtx-fmt" );
    TAC_HANDLE_ERROR( errors );

    mBlendState = Render::CreateBlendState( { .mSrcRGB = Render::BlendConstants::One,
                                             .mDstRGB = Render::BlendConstants::Zero,
                                             .mBlendRGB = Render::BlendMode::Add,
                                             .mSrcA = Render::BlendConstants::Zero,
                                             .mDstA = Render::BlendConstants::One,
                                             .mBlendA = Render::BlendMode::Add}, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "game-window-blend" );
    TAC_HANDLE_ERROR( errors );

    mAlphaBlendState = Render::CreateBlendState( { .mSrcRGB = Render::BlendConstants::SrcA,
                                                  .mDstRGB = Render::BlendConstants::OneMinusSrcA,
                                                  .mBlendRGB = Render::BlendMode::Add,
                                                  .mSrcA = Render::BlendConstants::Zero,
                                                  .mDstA = Render::BlendConstants::One,
                                                  .mBlendA = Render::BlendMode::Add},
                                                  TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mAlphaBlendState, "game-window-alpha-blend" );
    TAC_HANDLE_ERROR( errors );

    mDepthState = Render::CreateDepthState( { .mDepthTest = true,
                                              .mDepthWrite = true,
                                              .mDepthFunc = Render::DepthFunc::Less},
                                            TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "game-window-depth" );
    TAC_HANDLE_ERROR( errors );

    
    mRasterizerState = Render::CreateRasterizerState( { .mFillMode = Render::FillMode::Solid,
                                                        .mCullMode = Render::CullMode::None, // todo
                                                        .mFrontCounterClockwise = true,
                                                        .mScissor = true,
                                                        .mMultisample = false,
                                                      },
                                                      TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState, "game-window-rast" );
    TAC_HANDLE_ERROR( errors );

    mSamplerState = Render::CreateSamplerState( { .mFilter = Render::Filter::Linear }, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState, "game-window-samp" );
    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::Init( Errors& errors )
  {
    mDesktopWindowHandle = gCreation.CreateWindow( gGameWindowName );

    CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );



    mCenteredUnitCube = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/box.gltf",
                                                                0,
                                                                m3DvertexFormatDecls,
                                                                errors );
    TAC_HANDLE_ERROR( errors );

    mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                     0,
                                                     m3DvertexFormatDecls,
                                                     errors );
    TAC_HANDLE_ERROR( errors );

    mDebug3DDrawData = TAC_NEW Debug3DDrawData;

    PlayGame( errors );
    TAC_HANDLE_ERROR( errors );

    spriteShader = Render::CreateShader(  "3DSprite" , TAC_STACK_FRAME );
  }

  void CreationGameWindow::MousePickingGizmos()
  {
    if( gCreation.mSelectedEntities.empty() || !sGizmosEnabled )
      return;

    const v3 selectionGizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();

    const m4 invArrowRots[] = {
      m4::RotRadZ( 3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( -3.14f / 2.0f ), };

    for( int i = 0; i < 3; ++i )
    {
      // 1/3: inverse transform
      v3 modelSpaceRayPos3 = gCreation.mEditorCamera->mPos - selectionGizmoOrigin;
      v4 modelSpaceRayPos4 = v4( modelSpaceRayPos3, 1 );
      v3 modelSpaceRayDir3 = mWorldSpaceMouseDir;
      v4 modelSpaceRayDir4 = v4( mWorldSpaceMouseDir, 0 );

      // 2/3: inverse rotate
      const m4& invArrowRot = invArrowRots[ i ];
      modelSpaceRayPos4 = invArrowRot * modelSpaceRayPos4;
      modelSpaceRayPos3 = modelSpaceRayPos4.xyz();
      modelSpaceRayDir4 = invArrowRot * modelSpaceRayDir4;
      modelSpaceRayDir3 = modelSpaceRayDir4.xyz();

      // 3/3: inverse scale
      modelSpaceRayPos3 /= mArrowLen;

      bool hit = false;
      float dist = 0;
      mArrow->MeshModelSpaceRaycast( modelSpaceRayPos3, modelSpaceRayDir3, &hit, &dist );
      dist *= mArrowLen;
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;
      pickData.arrowAxis = i;
      pickData.closestDist = dist;
      pickData.pickedObject = PickedObject::WidgetTranslationArrow;
    }
  }

  void CreationGameWindow::MousePickingEntities()
  {
    for( Entity* entity : gCreation.mWorld->mEntities )
    {
      bool hit = false;
      float dist = 0;
      MousePickingEntity( entity, &hit, &dist );
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;
      pickData.closestDist = dist;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }
  }

  void CreationGameWindow::MousePickingSelection()
  {
    if( !Mouse::ButtonJustDown( Mouse::Button::MouseLeft ) )
      return;

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        v3 worldSpaceHitPoint = gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir;
        v3 gizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();
        v3 pickPoint = gCreation.mEditorCamera->mPos + mWorldSpaceMouseDir * pickData.closestDist;
        v3 arrowDir = {};
        arrowDir[ pickData.arrowAxis ] = 1;
        gCreation.mSelectedGizmo = true;
        gCreation.mTranslationGizmoDir = arrowDir;
        gCreation.mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
      } break;
      case PickedObject::Entity:
      {
        v3 worldSpaceHitPoint = gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir;
        v3 entityWorldOrigin = ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
        gCreation.mSelectedEntities.Select( pickData.closest );
        gCreation.mSelectedHitOffsetExists = true;
        gCreation.mSelectedHitOffset = worldSpaceHitPoint - entityWorldOrigin;
      } break;
      case PickedObject::None:
      {
        gCreation.mSelectedEntities.clear();
      } break;
    }
  }

  void CreationGameWindow::MousePickingAll()
  {
    pickData = {};

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( !IsWindowHovered( mDesktopWindowHandle ) )
      return;

    MousePickingEntities();

    MousePickingGizmos();

    MousePickingSelection();
  }

  void CreationGameWindow::MousePickingInit()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const float x = ( float )desktopWindowState->mX;
    const float y = ( float )desktopWindowState->mY;
    const v2 screenspaceCursorPos = Mouse::GetScreenspaceCursorPos();
    float xNDC = ( ( screenspaceCursorPos.x - x ) / w );
    float yNDC = ( ( screenspaceCursorPos.y - y ) / h );
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    const float aspect = w / h;
    const float theta = gCreation.mEditorCamera->mFovyrad / 2.0f;
    const float cotTheta = 1.0f / std::tan( theta );
    const float sX = cotTheta / aspect;
    const float sY = cotTheta;

    const m4 viewInv = m4::ViewInv( gCreation.mEditorCamera->mPos,
                                    gCreation.mEditorCamera->mForwards,
                                    gCreation.mEditorCamera->mRight,
                                    gCreation.mEditorCamera->mUp );
    const v3 viewSpaceMousePosNearPlane =
    {
      xNDC / sX,
      yNDC / sY,
      -1,
    };

    const v3 viewSpaceMouseDir = Normalize( viewSpaceMousePosNearPlane );
    const v4 viewSpaceMouseDir4 = v4( viewSpaceMouseDir, 0 );
    const v4 worldSpaceMouseDir4 = viewInv * viewSpaceMouseDir4;
    mWorldSpaceMouseDir = worldSpaceMouseDir4.xyz();
    mViewSpaceUnitMouseDir = viewSpaceMouseDir;
  }



  void CreationGameWindow::MousePickingEntityLight( const Light* light, bool* hit, float* dist )
  {
    //const Entity* entity = light->mEntity;
    //const v3 worldPos = entity->mWorldPosition;
    //const v3 viewPos = ( gCreation.mEditorCamera->View() * v4( entity->mWorldPosition, 1 ) ).xyz();

    //v3 worldSpaceMouseRayDir = mViewSpaceUnitMouseDir;

    //lightWidgetSize;

    const float t = RaySphere( gCreation.mEditorCamera->mPos,
                         mWorldSpaceMouseDir,
                         light->mEntity->mWorldPosition,
                         lightWidgetSize );
    if( t > 0 )
    {
      *hit = true;
      *dist = t;
    }
  }

  void CreationGameWindow::MousePickingEntityModel( const Model* model, bool* hit, float* dist )
  {
    const Entity* entity = model->mEntity;
    const Mesh* mesh = GamePresentationGetModelMesh( model );
    if( !mesh )
    {
      *hit = false;
      return;
    }

    bool transformInvExists;
    const m4 transformInv = m4::Inverse( entity->mWorldTransform, &transformInvExists );
    if( !transformInvExists )
    {
      *hit = false;
      return;
    }

    const Camera* camera = gCreation.mEditorCamera;

    const v3 modelSpaceMouseRayPos3 = ( transformInv * v4( camera->mPos, 1 ) ).xyz();
    const v3 modelSpaceMouseRayDir3 = Normalize( ( transformInv * v4( mWorldSpaceMouseDir, 0 ) ).xyz() );
    float modelSpaceDist;
    mesh->MeshModelSpaceRaycast( modelSpaceMouseRayPos3, modelSpaceMouseRayDir3, hit, &modelSpaceDist );

    // Recompute the distance by transforming the model space hit point into world space in order to
    // account for non-uniform scaling
    if( *hit )
    {
      const v3 modelSpaceHitPoint = modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * modelSpaceDist;
      const v3 worldSpaceHitPoint = ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz();
      *dist = Distance( camera->mPos, worldSpaceHitPoint );
    }
  }

  void CreationGameWindow::MousePickingEntity( const Entity* entity,
                                               bool* hit,
                                               float* dist )
  {
    if( const Model* model = Model::GetModel( entity ) )
    {
      MousePickingEntityModel( model, hit, dist );
      if( hit )
        return;
    }

    if( const Light* light = Light::GetLight( entity ) )
    {
      MousePickingEntityLight( light, hit, dist );
      if( hit )
        return;
    }
  }

  void CreationGameWindow::ComputeArrowLen()
  {
    if( gCreation.mSelectedEntities.empty() )
      return;
    m4 view = m4::View( gCreation.mEditorCamera->mPos,
                        gCreation.mEditorCamera->mForwards,
                        gCreation.mEditorCamera->mRight,
                        gCreation.mEditorCamera->mUp );
    v3 pos = gCreation.mSelectedEntities.GetGizmoOrigin();
    v4 posVS4 = view * v4( pos, 1 );
    float clip_height = std::abs( std::tan( gCreation.mEditorCamera->mFovyrad / 2.0f ) * posVS4.z * 2.0f );
    float arrowLen = clip_height * 0.2f;
    mArrowLen = arrowLen;
  }


  void CreationGameWindow::RenderEditorWidgetsPicking( Render::ViewHandle viewHandle )
  {
    v3 worldSpaceHitPoint = {};
    if( pickData.pickedObject != PickedObject::None )
    {
      worldSpaceHitPoint = gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir;
      mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );

      static double mouseMovement;
      Mouse::TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );
    }

  }

  void CreationGameWindow::RenderEditorWidgetsSelection( Render::ViewHandle viewHandle )
  {
    if( !sGizmosEnabled || gCreation.mSelectedEntities.empty() )
      return;


    Render::BeginGroup( "Selection", TAC_STACK_FRAME );

    const v3 selectionGizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();
    const m4 rots[] = {
      m4::RotRadZ( -3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( 3.14f / 2.0f ), };


    mDebug3DDrawData->DebugDraw3DCircle( selectionGizmoOrigin,
                                         gCreation.mEditorCamera->mForwards,
                                         mArrowLen );


    for( int i = 0; i < 3; ++i )
    {
      v4 color = { 0, 0, 0, 1 };
      color[ i ] = 1;

      // Widget Translation Arrow
      DefaultCBufferPerObject perObjectData;
      perObjectData.Color = color;
      perObjectData.World
        = m4::Translate( selectionGizmoOrigin )
        * rots[ i ]
        * m4::Scale( v3( 1, 1, 1 ) * mArrowLen );
      Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                    &perObjectData,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      AddDrawCall( mArrow, viewHandle );

      // Widget Scale Cube
      v3 axis = {};
      axis[ i ] = 1;
      perObjectData.World =
        m4::Translate( selectionGizmoOrigin ) *
        m4::Translate( axis * ( mArrowLen * 1.1f ) ) *
        rots[ i ] *
        m4::Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f );
      Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                    &perObjectData,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      AddDrawCall( mCenteredUnitCube, viewHandle );
    }
    Render::EndGroup( TAC_STACK_FRAME );
  }

  void CreationGameWindow::RenderEditorWidgetsLights( Render::ViewHandle viewHandle )
  {
    Render::BeginGroup( "lights", TAC_STACK_FRAME );
    struct : public LightVisitor
    {
      void operator()( Light* light ) override { mLights.push_back( light ); }
      Vector< const Light* > mLights;
    } lightVisitor;

    Graphics* graphics = GetGraphics( gCreation.mWorld );
    graphics->VisitLights( &lightVisitor );

    for( int iLight = 0; iLight < lightVisitor.mLights.size(); ++iLight )
    {
      const Light* light = lightVisitor.mLights[ iLight ];
      const char* groupName = FrameMemoryPrintf( "editor light %i", iLight );


      m4 world = light->mEntity->mWorldTransform;;
      world.m00 = lightWidgetSize;
      DefaultCBufferPerObject perObjectData{ .World = world, .Color = v4( 1, 1, 1, 1 )};

      Errors errors;
      Render::TextureHandle textureHandle = TextureAssetManager::GetTexture( "assets/editor/light.png", errors );

      Render::BeginGroup( groupName, TAC_STACK_FRAME );
      Render::SetShader( spriteShader );
      Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, 6 );
      Render::SetIndexBuffer( Render::IndexBufferHandle(), 0, 0 );
      //Render::SetBlendState( mBlendState );
      Render::SetBlendState( mAlphaBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerState );
      Render::SetDepthState( mDepthState );
      Render::SetVertexFormat( Render::VertexFormatHandle() );
      Render::SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
      Render::SetTexture( textureHandle );
      Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                    &perObjectData,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::Submit( viewHandle, TAC_STACK_FRAME );
      Render::EndGroup( TAC_STACK_FRAME );
    }

    Render::Submit( viewHandle, TAC_STACK_FRAME );
    Render::EndGroup( TAC_STACK_FRAME );
  }

  void CreationGameWindow::RenderEditorWidgets( Render::ViewHandle viewHandle )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    Render::BeginGroup( "Editor Widgets", TAC_STACK_FRAME );
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const DefaultCBufferPerFrame perFrameData = GetPerFrame( w, h );
    Render::UpdateConstantBuffer( DefaultCBufferPerFrame::Handle,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    RenderEditorWidgetsPicking( viewHandle );
    RenderEditorWidgetsSelection( viewHandle );
    RenderEditorWidgetsLights( viewHandle );
    Render::EndGroup( TAC_STACK_FRAME );
  }

  void CreationGameWindow::PlayGame( Errors& errors )
  {
    if( mSoul )
      return;
    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
    mSoul = ghost;
  }

  void CreationGameWindow::ImGuiOverlay( Errors& errors )
  {
    ImGuiSetNextWindowSize( { 300, 405 } );
    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiBegin( "gameplay overlay" );
    mCloseRequested |= ImGuiButton( "Close Window" );

    static bool mHideUI = false;
    if( !mHideUI )
    {
      ImGuiCheckbox( "Draw grid", &drawGrid );
      ImGuiCheckbox( "hide ui", &mHideUI );
      ImGuiCheckbox( "draw gizmos", &sGizmosEnabled );

      if( mSoul )
      {
        if( ImGuiButton( "End simulation" ) )
        {
          delete mSoul;
          mSoul = nullptr;
        }
      }
      else
      {
        if( ImGuiButton( "Begin simulation" ) )
        {
          PlayGame( errors );
          TAC_HANDLE_ERROR( errors );
        }
      }

      if( ImGuiCollapsingHeader( "Camera" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        Camera* cam = gCreation.mEditorCamera;
        if( ImGuiCollapsingHeader( "transform" ) )
        {
          TAC_IMGUI_INDENT_BLOCK;
          if( ImGuiCollapsingHeader( "pos" ) )
            ImGuiDragFloat3( "cam pos", cam->mPos.data() );
          if( ImGuiCollapsingHeader( "forward" ) )
            ImGuiDragFloat3( "cam forward", cam->mForwards.data() );
          if( ImGuiCollapsingHeader( "right" ) )
            ImGuiDragFloat3( "cam right", cam->mRight.data() );
          if( ImGuiCollapsingHeader( "up" ) )
            ImGuiDragFloat3( "cam up", cam->mUp.data() );
        }
        if( ImGuiCollapsingHeader( "clipping planes" ) )
        {
          TAC_IMGUI_INDENT_BLOCK;
          ImGuiDragFloat( "cam far", &cam->mFarPlane );
          ImGuiDragFloat( "cam near", &cam->mNearPlane );
        }
        ImGuiDragFloat( "cam fovyrad", &cam->mFovyrad );
        if( ImGuiButton( "cam snap pos" ) )
        {
          cam->mPos.x = ( float )( int )cam->mPos.x;
          cam->mPos.y = ( float )( int )cam->mPos.y;
          cam->mPos.z = ( float )( int )cam->mPos.z;
        }
        if( ImGuiButton( "cam snap dir" ) )
          cam->SetForwards( SnapToUnitDir( cam->mForwards ) );
      }

      if( ShellGetElapsedSeconds() < mStatusMessageEndTime )
      {
        ImGuiText( mStatusMessage );
      }

    }
    ImGuiEnd();
  }


  static void CameraWASDControlsPan( Camera* camera )
  {
    v3 combinedDir = {};
     
    struct PanKeyDir
    {
      Keyboard::Key key;
      v3 dir;
    };
    
    const PanKeyDir keyDirs[] =
    {
      { Keyboard::Key::W, camera->mForwards},
      { Keyboard::Key::A, -camera->mRight},
      { Keyboard::Key::S, -camera->mForwards},
      { Keyboard::Key::D, camera->mRight},
      { Keyboard::Key::Q, -camera->mUp },
      { Keyboard::Key::E, camera->mUp},
    };
    for( const PanKeyDir& keyDir : keyDirs )
      if( KeyboardIsKeyDown( keyDir.key ) )
        combinedDir += keyDir.dir;
    if( combinedDir == v3( 0, 0, 0 ) )
      return;
    camera->mPos += combinedDir * sWASDCameraPanSpeed;
  }

  static void CameraWASDControlsOrbit( Camera* camera, const v3 orbitCenter )
  {
    const float vertLimit = 0.1f;

    struct OrbitKeyDir
    {
      Keyboard::Key key;
      v3            spherical;
    };
    
    OrbitKeyDir keyDirs[] =
    {
      { Keyboard::Key::W, v3( 0, -1, 0 ) },
      { Keyboard::Key::A, v3( 0,  0, 1 ) },
      { Keyboard::Key::S, v3( 0, 1, 0 ) },
      { Keyboard::Key::D, v3( 0, 0, -1 ) }
    };


    v3 camOrbitSphericalOffset = {};
    for( const OrbitKeyDir& keyDir : keyDirs )
      if( KeyboardIsKeyDown( keyDir.key ) )
        camOrbitSphericalOffset += keyDir.spherical;
    if( camOrbitSphericalOffset == v3( 0, 0, 0 ) )
      return;

    v3 camOrbitSpherical = CartesianToSpherical( camera->mPos - orbitCenter );
    camOrbitSpherical += camOrbitSphericalOffset * sWASDCameraOrbitSpeed;
    camOrbitSpherical.y = Clamp( camOrbitSpherical.y, vertLimit, 3.14f - vertLimit );

    camera->mPos = orbitCenter + SphericalToCartesian( camOrbitSpherical );

    if( sWASDCameraOrbitSnap )
    {
      camera->SetForwards( orbitCenter - camera->mPos );
    }
    else
    {
      v3 dirCart = camera->mForwards;
      v3 dirSphe = CartesianToSpherical( dirCart );
      dirSphe.y += -camOrbitSphericalOffset.y * sWASDCameraOrbitSpeed;
      dirSphe.z += camOrbitSphericalOffset.z * sWASDCameraOrbitSpeed;
      dirSphe.y = Clamp( dirSphe.y, vertLimit, 3.14f - vertLimit );
      v3 newForwards = SphericalToCartesian( dirSphe );
      camera->SetForwards( newForwards );
    }
  }

  static void CameraWASDControls( Camera* camera )
  {
    if( gCreation.mSelectedEntities.empty() )
    {
      CameraWASDControlsPan( camera );
    }
    else
    {
      CameraWASDControlsOrbit( camera, gCreation.mSelectedEntities.GetGizmoOrigin() );
    }
  }

  static void CameraUpdateSaved()
  {
    static AssetPathString savedPrefabPath;
    static Camera savedCamera;

    const AssetPathString loadedPrefab = PrefabGetLoaded();
    if( loadedPrefab != savedPrefabPath )
    {
      savedPrefabPath = loadedPrefab;
      savedCamera = *gCreation.mEditorCamera;
    }

    const bool cameraSame =
      savedCamera.mPos == gCreation.mEditorCamera->mPos  &&
      savedCamera.mForwards == gCreation.mEditorCamera->mForwards &&
      savedCamera.mRight == gCreation.mEditorCamera->mRight &&
      savedCamera.mUp == gCreation.mEditorCamera->mUp;
    if( cameraSame )
      return;

    savedCamera = *gCreation.mEditorCamera;
    PrefabSaveCamera( gCreation.mEditorCamera );
  }

  void CreationGameWindow::CameraUpdateControls()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    if( !IsWindowHovered( mDesktopWindowHandle ) )
      return;
    const Camera oldCamera = *gCreation.mEditorCamera;

    const v2 mouseDeltaPos = Mouse::GetMouseDeltaPos();
    if( Mouse::ButtonIsDown( Mouse::Button::MouseRight ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float pixelsPerDeg = 400.0f / 90.0f;
      const float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
      const v2 angleRadians = mouseDeltaPos * radiansPerPixel;

      if( angleRadians.x != 0 )
      {
        m3 matrix = m3::RotRadAngleAxis( -angleRadians.x, gCreation.mEditorCamera->mUp );
        gCreation.mEditorCamera->mForwards = matrix * gCreation.mEditorCamera->mForwards;
        gCreation.mEditorCamera->mRight = Cross( gCreation.mEditorCamera->mForwards,
                                                 gCreation.mEditorCamera->mUp );
      }

      if( angleRadians.y != 0 )
      {
        m3 matrix = m3::RotRadAngleAxis( -angleRadians.y, gCreation.mEditorCamera->mRight );
        gCreation.mEditorCamera->mForwards = matrix * gCreation.mEditorCamera->mForwards;
        gCreation.mEditorCamera->mUp = Cross( gCreation.mEditorCamera->mRight,
                                              gCreation.mEditorCamera->mForwards );
      }

      // Snapping right.y to the x-z plane prevents the camera from tilting side-to-side.
      gCreation.mEditorCamera->mForwards.Normalize();
      gCreation.mEditorCamera->mRight.y = 0;
      gCreation.mEditorCamera->mRight.Normalize();
      gCreation.mEditorCamera->mUp = Cross( gCreation.mEditorCamera->mRight,
                                            gCreation.mEditorCamera->mForwards );
      gCreation.mEditorCamera->mUp.Normalize();
    }

    if( Mouse::ButtonIsDown( Mouse::Button::MouseMiddle ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float unitsPerPixel = 5.0f / 100.0f;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mRight *
        -mouseDeltaPos.x *
        unitsPerPixel;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mUp *
        mouseDeltaPos.y *
        unitsPerPixel;
    }

    const int mouseDeltaScroll = Mouse::GetMouseDeltaScroll();
    if( mouseDeltaScroll )
    {
      float unitsPerTick = 1.0f;

      if( gCreation.mSelectedEntities.size() )
      {
        const v3 origin = gCreation.mSelectedEntities.GetGizmoOrigin();
        unitsPerTick = Distance( origin, gCreation.mEditorCamera->mPos ) * 0.1f;
      }

      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mForwards *
        ( float )mouseDeltaScroll *
        unitsPerTick;
    }

    CameraWASDControls( gCreation.mEditorCamera );
  }

  void CreationGameWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );

    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    DesktopAppMoveControls( mDesktopWindowHandle );

    const v2 size = desktopWindowState->GetSizeV2();
    const Render::Viewport viewport(size);
    const Render::ScissorRect scissorRect(size);

    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, viewport );
    Render::SetViewScissorRect( viewHandle, scissorRect );


    if( mSoul )
    {
      //static bool once;
      //if( !once )
      //{
      //  once = true;
      //  Entity* entity = gCreation.CreateEntity();
      //  entity->mName = "Starry-eyed girl";
      //  entity->mPosition = {}; // { 4.5f, -4.0f, -0.5f };
      //  auto model = ( Model* )entity->AddNewComponent( ComponentRegistryEntryIndex::Model );
      //  model->mModelPath = "assets/editor/Box.gltf";
      //}
      mSoul->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( drawGrid )
      mDebug3DDrawData->DebugDraw3DGrid();

    MousePickingInit();
    CameraUpdateSaved();
    CameraUpdateControls();
    ComputeArrowLen();
    MousePickingAll();

    GamePresentationRender( gCreation.mWorld,
                            gCreation.mEditorCamera,
                            desktopWindowState->mWidth,
                            desktopWindowState->mHeight,
                            viewHandle );

    RenderEditorWidgets( viewHandle );

    VoxelGIPresentationRender( gCreation.mWorld,
                               gCreation.mEditorCamera,
                               desktopWindowState->mWidth,
                               desktopWindowState->mHeight,
                               viewHandle );

    mDebug3DDrawData->DebugDraw3DToTexture( viewHandle,
                                            gCreation.mEditorCamera,
                                            desktopWindowState->mWidth,
                                            desktopWindowState->mHeight,
                                            errors );

    UpdateGizmo();
    ImGuiOverlay( errors );

    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::UpdateGizmo()
  {
    if( !gCreation.mSelectedGizmo )
      return;

    const v3 origin = gCreation.mSelectedEntities.GetGizmoOrigin();
    float gizmoMouseDist;
    float secondDist;
    ClosestPointTwoRays( gCreation.mEditorCamera->mPos,
                         mWorldSpaceMouseDir,
                         origin,
                         gCreation.mTranslationGizmoDir,
                         &gizmoMouseDist,
                         &secondDist );
    const v3 translate = gCreation.mTranslationGizmoDir *
      ( secondDist - gCreation.mTranslationGizmoOffset );
    for( Entity* entity : gCreation.mSelectedEntities )
    {
      entity->mRelativeSpace.mPosition += translate;
    }
    //for( Prefab* prefab : gCreation.mSelectedPrefabs )
    //{
    //  prefab->mPosition += translate;
    //}
    if( !Mouse::ButtonIsDown( Mouse::Button::MouseLeft ) )
    {
      gCreation.mSelectedGizmo = false;
    }

  }
}
