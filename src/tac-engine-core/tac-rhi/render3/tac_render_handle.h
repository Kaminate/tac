#pragma once


namespace Tac::Render
{
  struct IHandle
  {
    static constexpr int kInvalidIndex{ -1 };

    IHandle( int i = kInvalidIndex );
    int GetIndex() const;
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
  };

  const char* HandleTypeToString( HandleType );

  struct SwapChainHandle : public IHandle { SwapChainHandle ( int i = kInvalidIndex ) : IHandle{ i } {} };
  struct PipelineHandle  : public IHandle { PipelineHandle  ( int i = kInvalidIndex ) : IHandle{ i } {} };
  struct ProgramHandle   : public IHandle { ProgramHandle   ( int i = kInvalidIndex ) : IHandle{ i } {} };
  struct BufferHandle    : public IHandle { BufferHandle    ( int i = kInvalidIndex ) : IHandle{ i } {} };
  struct TextureHandle   : public IHandle { TextureHandle   ( int i = kInvalidIndex ) : IHandle{ i } {} };
  struct SamplerHandle   : public IHandle { SamplerHandle   ( int i = kInvalidIndex ) : IHandle{ i } {} };

  // Basically a variant that can hold any type of gpu resource
  struct ResourceHandle : public IHandle
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
    HandleType GetHandleType() const;

  private:
    HandleType mHandleType {};
  };

} // namespace Tac::Render

