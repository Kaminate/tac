#include "tac_render_backend.h" // self-inc

namespace Tac::Render
{
}

namespace Tac
{
  struct IdMgr
  {
    int Alloc();
    void Free( int );

  private:
    IdCollection sCollection;
    std::mutex   sMtx;
  };

  int  IdMgr::Alloc()
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMtx );
    return sCollection.Alloc();
  }

  void IdMgr::Free( int i )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sMtx );
    return sCollection.Free( i );
  }

  // -----------------------------------------------------------------------------------------------

  static IdMgr sIdMgrFB;
  static IdMgr sIdMgrPipeline;
  static IdMgr sIdMgrProgram;
  static IdMgr sIdMgrBuffer;
  static IdMgr sIdMgrTexture;

  Render::FBHandle        Render::AllocFBHandle()        { return FBHandle( sIdMgrFB.Alloc() ); }
  Render::PipelineHandle  Render::AllocPipelineHandle()  { return PipelineHandle( sIdMgrPipeline.Alloc() ); }
  Render::ProgramHandle   Render::AllocProgramHandle()   { return ProgramHandle( sIdMgrProgram.Alloc() ); }
  Render::BufferHandle    Render::AllocBufferHandle()    { return BufferHandle( sIdMgrBuffer.Alloc() ); }
  Render::TextureHandle   Render::AllocTextureHandle()   { return TextureHandle( sIdMgrTexture.Alloc() ); }

  void            Render::FreeHandle( FBHandle h )       { sIdMgrFB.Free( h.GetIndex() ); }
  void            Render::FreeHandle( PipelineHandle h ) { sIdMgrPipeline.Free( h.GetIndex() ); }
  void            Render::FreeHandle( ProgramHandle h )  { sIdMgrProgram.Free( h.GetIndex() ); }
  void            Render::FreeHandle( BufferHandle h )   { sIdMgrBuffer.Free( h.GetIndex() ); }
  void            Render::FreeHandle( TextureHandle h )  { sIdMgrTexture.Free( h.GetIndex() ); }
}
