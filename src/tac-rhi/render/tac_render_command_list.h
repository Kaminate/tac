#pragma once

namespace Tac::Render
{

  struct ICommandList
  {
    virtual void Draw() {}
    virtual void SetViewports() {}
    virtual void SetScissorRects() {}
    virtual void SetPrimitiveTopology() {}
    virtual void SetRenderTargets() {}

    virtual void SetPipelineState() {}
    virtual void SetDescriptorHeaps() {}
    virtual void SetRootCBV() {}
    virtual void SetRootSRV() {}
    virtual void SetRootSignature() {}
    virtual void SetRootDescriptorTable() {}
  };


} // namespace Tac::Render
