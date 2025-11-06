#include "tac_render_tutorial.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle

#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct HelloTexture : public App
  {
    HelloTexture( App::Config );

  protected:
    void Init( Errors& ) override;
    void Render( RenderParams, Errors& ) override;

  private:

    WindowHandle           mWindowHandle   {};
    Render::BufferHandle   mVtxBuf         {};
    Render::ProgramHandle  mShader         {};
    Render::PipelineHandle mPipeline       {};
    Render::TexFmt         mColorFormat    {};
    Render::TexFmt         mDepthFormat    {};
    Render::SamplerHandle  mSampler        {};
    Render::TextureHandle  mTexture        {};
    Render::IShaderVar*    mShaderVtxBufs  {};
    Render::IShaderVar*    mShaderSamplers {};
    Render::IShaderVar*    mShaderTextures {};
    int                    mVtxCount       {};
#if 1
    const char*            mTexPath        { "assets/essential/are_ya_winnin_son.png" };
    Render::Filter         mFilter         { Render::Filter::Linear };
#else
    const char*            mTexPath        { "assets/essential/image_loader_test.png" };
    //const char*            mTexPath      { "assets/essential/1024white.png" };
    Render::Filter         mFilter         { Render::Filter::Point };
#endif
  };

} // namespace Tac

