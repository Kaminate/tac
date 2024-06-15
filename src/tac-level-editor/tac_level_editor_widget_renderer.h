#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-level-editor/tac_level_editor_mouse_picking.h"

#define TAC_IS_WIDGET_RENDERER_ENABLED() 1

#if TAC_IS_WIDGET_RENDERER_ENABLED()

namespace Tac
{
  struct WidgetRenderer
  {
    void Init( CreationMousePicking*,
               GizmoMgr*,
               Errors& );
    void Uninit();

    void RenderTranslationWidget( Render::IContext*,
                                  WindowHandle,
                                  const Camera*,
                                  Errors& );

  private:

    void UpdatePerFrame( Render::IContext*,
                         WindowHandle,
                         const Camera*,
                         Errors& );

    void UpdatePerObject( Render::IContext*, int, Errors& );

    v4 GetAxisColor( int );
    m4 GetAxisWorld( int );

    Render::PipelineParams        GetPipelineParams();

    Render::ProgramHandle         m3DShader                 {};
    Render::PipelineHandle        m3DPipeline               {};
    Render::BufferHandle          mBufferPerFrame           {};
    Render::BufferHandle          mBufferPerObj             {};
    Render::IShaderVar*           mShaderPerFrame           {};
    Render::IShaderVar*           mShaderPerObj             {};

    Mesh*                         mArrow                    {};

    CreationMousePicking*         mMousePicking             {};
    GizmoMgr*                     mGizmoMgr                 {};

  };
}
#endif
