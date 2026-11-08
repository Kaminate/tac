#pragma once


namespace Tac::Render
{
  struct IHandle
  {
    static constexpr int kInvalidIndex{ -1 };

    IHandle( int i = kInvalidIndex );
    auto GetIndex() const -> int;
    bool IsValid() const;

  private:
    int mIndex;
  };

  enum class HandleType
  {
    kUnknown = 0,
    kSwapChain,
    kPipeline,
    kProgram,
    kBuffer,
    kTexture,
    kSampler,
    kCount,
  };

  auto HandleTypeToString( HandleType ) -> const char*;

  struct SwapChainHandle : public IHandle { SwapChainHandle() = default; explicit SwapChainHandle ( int i  ) : IHandle{ i } {} };
  struct PipelineHandle  : public IHandle { PipelineHandle() = default;  explicit PipelineHandle  ( int i  ) : IHandle{ i } {} };
  struct ProgramHandle   : public IHandle { ProgramHandle() = default;   explicit ProgramHandle   ( int i  ) : IHandle{ i } {} };
  struct BufferHandle    : public IHandle { BufferHandle() = default;    explicit BufferHandle    ( int i  ) : IHandle{ i } {} };
  struct TextureHandle   : public IHandle { TextureHandle() = default;   explicit TextureHandle   ( int i  ) : IHandle{ i } {} };
  struct SamplerHandle   : public IHandle { SamplerHandle() = default;   explicit SamplerHandle   ( int i  ) : IHandle{ i } {} };

  // Basically a variant that can hold any type of gpu resource
  struct ResourceHandle// : public IHandle
  {
    ResourceHandle() = default;
    ResourceHandle( HandleType, IHandle );
    ResourceHandle( SwapChainHandle ); 
    ResourceHandle( PipelineHandle );
    ResourceHandle( ProgramHandle );
    ResourceHandle( BufferHandle );
    ResourceHandle( TextureHandle );
    ResourceHandle( SamplerHandle );
    operator SwapChainHandle() const;
    operator PipelineHandle() const;
    operator ProgramHandle() const;
    operator BufferHandle() const;
    operator TextureHandle() const;
    operator SamplerHandle() const;

    auto GetHandle() const -> IHandle        { return mHandle; }
    auto GetHandleType() const -> HandleType { return mHandleType;}
    bool IsValid() const                     { return mHandle.IsValid(); }
    auto GetIndex() const                    { return mHandle.GetIndex(); }
    operator IHandle() const                 { return mHandle; }

  private:
    HandleType mHandleType {};
    IHandle    mHandle     {};
  };

} // namespace Tac::Render

