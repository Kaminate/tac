#include "tac_render_tutorial.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct HelloTriangle : public App
  {
    HelloTriangle( App::Config );

  protected:
    void Init( Errors& ) override;
    void Render( RenderParams, Errors& ) override;

  private:
    void InitWindow( Errors& );
    void InitVertexBuffer( Errors& );
    void InitShader( Errors& );
    void InitRootSig( Errors& );

    WindowHandle           mWindowHandle        {};
    Render::BufferHandle   mVtxBuf              {};
    Render::ProgramHandle  mShaderBindless      {};
    Render::ProgramHandle  mShaderInputLayout   {};
    Render::PipelineHandle mPipelineBindless    {};
    Render::PipelineHandle mPipelineInputLayout {};
    Render::TexFmt         mColorFormat         {};
    Render::TexFmt         mDepthFormat         {};
    bool                   mBindless            { true };
  };

} // namespace Tac

