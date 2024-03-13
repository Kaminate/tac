#include "tac_render_command_handler.h" // self-inc

#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/command/tac_render_command_buffer.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac::Render
{
  static bool gVerbose;

  // -----------------------------------------------------------------------------------------------
  template< typename CommandData >
  using CommandCallback = void ( Renderer::* )( CommandData*, Errors& );

  template< typename Handle >
  using DestroyCommandCallback = void ( Renderer::* )( Handle, Errors& );



  template< typename CommandData >
  struct CommandHandler : public CommandHandlerBase
  {

    void Invoke( Renderer* renderer,
                 CommandBufferIterator* commandBufferIterator,
                 Errors& errors ) const override;
    
    CommandCallback< CommandData > mCallback = nullptr;

  };

  // -----------------------------------------------------------------------------------------------



  template< typename CommandData >
  void CommandHandler<CommandData>::Invoke( Renderer* renderer,
                                            CommandBufferIterator* commandBufferIterator,
                                            Errors& errors ) const
  {
    auto commandData = commandBufferIterator->PopCommandData< CommandData >();
    TAC_CALL( ( renderer->*mCallback )( commandData, errors ));
  }

  // -----------------------------------------------------------------------------------------------

  template< typename Handle >
  struct CommandHandlerDestroy : public CommandHandlerBase
  {

    void Invoke( Renderer* renderer,
                 CommandBufferIterator* commandBufferIterator,
                 Errors& errors ) const override;
    

    DestroyCommandCallback<Handle> mCallback = nullptr;
  };

  // -----------------------------------------------------------------------------------------------


  template< typename THandle >
  void CommandHandlerDestroy<THandle>::Invoke( Renderer* renderer,
                                              CommandBufferIterator* iter,
                                              Errors& errors ) const
  {
    const CommandDataDestroy* commandData = iter->PopCommandData< CommandDataDestroy >();
    const THandle handle( commandData->mIndex );

    if( !handle.Handle::IsValid() )
      return;

    TAC_CALL( ( renderer->*mCallback )( handle, errors ));
  }

  // -----------------------------------------------------------------------------------------------

  void CommandHandlers::Add( const CommandType commandType,
                             const CommandHandlerBase* commandHandler )
  {
    mCommandHandlers[ ( int )commandType ] = commandHandler;
  }

  void CommandHandlers::Invoke( Renderer* renderer,
                                const CommandType comandType,
                                CommandBufferIterator* commandBufferIterator,
                                Errors& errors ) const
  {
    TAC_ASSERT_INDEX( comandType, CommandType::Count );
    const CommandHandlerBase* commandHandler = mCommandHandlers[ ( int )comandType ];

    if( gVerbose )
    {
      OS::OSDebugPrintLine( String() + commandHandler->mName + "::End" );
    }

    commandHandler->Invoke( renderer, commandBufferIterator, errors );

    if( gVerbose )
    {
      OS::OSDebugPrintLine(String() + commandHandler->mName + "::Begin");
    }
  }


  // Used in CommandHandlers() REG macro
  template< typename CommandData >
  static const CommandHandler< CommandData >* MakeCommandHandler( CommandCallback<CommandData> fn,
                                                            const char* name  )
  {
    static CommandHandler< CommandData > handler;
    handler.mName = name;
    handler.mCallback = fn;
    return &handler;
  }

  // Used in CommandHandlers() REG macro
  template< typename Handle >
  static const CommandHandlerDestroy< Handle >* MakeCommandHandlerDestroy( DestroyCommandCallback< Handle > fn,
                                                                     const char* name )
  {
    static CommandHandlerDestroy< Handle > handler;
    handler.mName = name;
    handler.mCallback = fn;
    return &handler;
  }

  CommandHandlers::CommandHandlers()
  {

    // For example, 
    //   REG( CommandHandler, CommandType::CreateBlendState, Renderer::AddBlendState );
    //
    // becomes
    //   Add( CommandType::CreateBlendState, MakeCommandHandler

#define REG( Handler, CmdType, RenderFn )                                                         \
    {                                                                                             \
      const CommandHandlerBase* pHandler = TAC_CONCAT( Make, Handler ) ( &RenderFn, # CmdType  ); \
      Add( CmdType, pHandler );                                                                   \
    }

    REG( CommandHandler, CommandType::CreateBlendState, Renderer::AddBlendState );
    REG( CommandHandler, CommandType::CreateConstantBuffer, Renderer::AddConstantBuffer );
    REG( CommandHandler, CommandType::CreateDepthState, Renderer::AddDepthState );
    REG( CommandHandler, CommandType::CreateFramebuffer, Renderer::AddFramebuffer );
    REG( CommandHandler, CommandType::CreateIndexBuffer, Renderer::AddIndexBuffer );
    REG( CommandHandler, CommandType::CreateMagicBuffer, Renderer::AddMagicBuffer );
    REG( CommandHandler, CommandType::CreateRasterizerState, Renderer::AddRasterizerState );
    REG( CommandHandler, CommandType::CreateSamplerState, Renderer::AddSamplerState );
    REG( CommandHandler, CommandType::CreateShader, Renderer::AddShader );
    REG( CommandHandler, CommandType::CreateTexture, Renderer::AddTexture );
    REG( CommandHandler, CommandType::CreateVertexBuffer, Renderer::AddVertexBuffer );
    REG( CommandHandler, CommandType::CreateVertexFormat, Renderer::AddVertexFormat );
    REG( CommandHandler, CommandType::ResizeFramebuffer, Renderer::ResizeFramebuffer );
    REG( CommandHandler, CommandType::SetRenderObjectDebugName, Renderer::SetRenderObjectDebugName );
    REG( CommandHandler, CommandType::UpdateIndexBuffer, Renderer::UpdateIndexBuffer );
    REG( CommandHandler, CommandType::UpdateTextureRegion, Renderer::UpdateTextureRegion );
    REG( CommandHandler, CommandType::UpdateVertexBuffer, Renderer::UpdateVertexBuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyBlendState, Renderer::RemoveBlendState );
    REG( CommandHandlerDestroy, CommandType::DestroyConstantBuffer, Renderer::RemoveConstantBuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyDepthState, Renderer::RemoveDepthState );
    REG( CommandHandlerDestroy, CommandType::DestroyFramebuffer, Renderer::RemoveFramebuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyIndexBuffer, Renderer::RemoveIndexBuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyRasterizerState, Renderer::RemoveRasterizerState );
    REG( CommandHandlerDestroy, CommandType::DestroySamplerState, Renderer::RemoveSamplerState );
    REG( CommandHandlerDestroy, CommandType::DestroyShader, Renderer::RemoveShader );
    REG( CommandHandlerDestroy, CommandType::DestroyTexture, Renderer::RemoveTexture );
    REG( CommandHandlerDestroy, CommandType::DestroyMagicBuffer, Renderer::RemoveMagicBuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyVertexBuffer, Renderer::RemoveVertexBuffer );
    REG( CommandHandlerDestroy, CommandType::DestroyVertexFormat, Renderer::RemoveVertexFormat );
#undef REG
    for( const CommandHandlerBase* handler : mCommandHandlers )
    {
      TAC_ASSERT( handler );
    }
  }
} // namespace Tac::Render

