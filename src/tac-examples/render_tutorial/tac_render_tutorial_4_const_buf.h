#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"

namespace Tac
{
  struct HelloConstBuf : public App
  {
    HelloConstBuf( App::Config );

  protected:
    void Init( InitParams, Errors& ) override;
    void Render( RenderParams, Errors& ) override;

  private:

    WindowHandle                    mWindowHandle          {};
    Render::BufferHandle            mVtxBuf                {};
    Render::BufferHandle            mConstantBuf           {};
    Render::ProgramHandle           mShader                {};
    Render::PipelineHandle          mPipeline              {};
    Render::TexFmt                  mColorFormat           {};
    Render::TexFmt                  mDepthFormat           {};
    Render::IShaderVar*             mShaderVtxBufs         {};
    Render::IShaderVar*             mShaderConstantBuffer  {};
    Render::IBindlessArray*         mBindlessArray         {};
    Render::IBindlessArray::Binding mBindlessVtxBufBinding {};
    int                             mVtxCount              {};
  };
} // namespace Tac

