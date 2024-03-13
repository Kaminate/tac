#include "src/shell/windows/renderer/dx11/tac_dx11_namer.h" // self-inc

#include "src/shell/windows/renderer/dx11/tac_renderer_dx11.h" // GetInfoQueue
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"
#include "src/shell/tac_desktop_app.h" // IsMainThread
#include "src/shell/tac_desktop_app_threads.h"

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h" // TAC_ASSERT


namespace Tac::Render
{


  // -----------------------------------------------------------------------------------------------

  struct Namer
  {
    virtual void SetName(const StringView&) = 0;
    virtual String GetName() = 0;
  };

  struct DXGINamer : public Namer
  {
    DXGINamer( IDXGIObject* );
    void SetName( const StringView& ) override;
    String GetName() override;
    IDXGIObject* mObj = nullptr;
  };

  struct ID3D11DeviceChildNamer : public Namer
  {
    ID3D11DeviceChildNamer( ID3D11DeviceChild* );
    void SetName( const StringView& ) override;
    String GetName() override;
    ID3D11DeviceChild* mDeviceChild = nullptr;
  };

  // -----------------------------------------------------------------------------------------------

  DXGINamer::DXGINamer( IDXGIObject* obj ) : mObj( obj ) {}

  void DXGINamer::SetName( const StringView& name )
  {
    DXGISetObjectName( mObj, name );
  }

  String DXGINamer::GetName()
  {
    return DXGIGetObjectName( mObj );
  }

  // -----------------------------------------------------------------------------------------------

  ID3D11DeviceChildNamer::ID3D11DeviceChildNamer( ID3D11DeviceChild* deviceChild )
    : mDeviceChild( deviceChild )
  {
  }

  void ID3D11DeviceChildNamer::SetName( const StringView& name ) 
  {
    ID3D11DeviceChildSetName( mDeviceChild, name );
  }

  String ID3D11DeviceChildNamer::GetName() 
  {
    return ID3D11DeviceChildGetName( mDeviceChild );
  }

  // -----------------------------------------------------------------------------------------------

  struct ScopedDXFilter
  {
    ScopedDXFilter() = default;
    ScopedDXFilter( D3D11_MESSAGE_ID* , int );
    ScopedDXFilter( D3D11_MESSAGE_ID );
    ~ScopedDXFilter();
    void Push( D3D11_MESSAGE_ID );
    void Push( D3D11_MESSAGE_ID* , int );

  private:

    FixedVector< D3D11_MESSAGE_ID, 100 > denyList;
    int pushCount = 0;
  };

  // -----------------------------------------------------------------------------------------------

  static void SetTDebugName( Namer* namer,
                             const StringView& name,
                             const StringView& suffix )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    TAC_ASSERT( name.size() );
    TAC_ASSERT( suffix.size() );
    if constexpr( !IsDebugMode )
      return;

    const String oldName = namer->GetName();

    String prefix;

    ScopedDXFilter filter;

    const bool kAppendInsteadOfReplace = false;

    if( !oldName.empty() )
    {
      if( kAppendInsteadOfReplace )
        prefix = oldName + ", and ";
      else
        filter.Push( { D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS } );
    }

    const String newname = prefix + String(name) + String(" ") + String(suffix);

    namer->SetName( newname );
  }



  String ID3D11DeviceChildGetName(ID3D11DeviceChild* deviceChild)
  {
    TAC_ASSERT( deviceChild );
    const int kBufSize = 256;
    char buf[ kBufSize ]{};
    UINT size = kBufSize;
    const HRESULT getHr = deviceChild->GetPrivateData( WKPDID_D3DDebugObjectName, &size, buf );
    TAC_ASSERT( SUCCEEDED( getHr ) || getHr == DXGI_ERROR_NOT_FOUND );
    return buf;
  }


  void ID3D11DeviceChildSetName(ID3D11DeviceChild* deviceChild, const StringView& name)
  {
    TAC_ASSERT( deviceChild );
    const UINT n = ( UINT )name.size();
    const void* s = name.c_str();
    const HRESULT setHr = deviceChild->SetPrivateData( WKPDID_D3DDebugObjectName, n, s );
    TAC_ASSERT( SUCCEEDED( setHr ) );
  }

  // -----------------------------------------------------------------------------------------------

  ScopedDXFilter::ScopedDXFilter( D3D11_MESSAGE_ID* msgs, int n ) { Push( msgs, n ); }

  ScopedDXFilter::ScopedDXFilter( D3D11_MESSAGE_ID msg ) { Push( msg ); }

  ScopedDXFilter::~ScopedDXFilter()
  {
    ID3D11InfoQueue* mInfoQueueDEBUG = RendererDirectX11::GetInfoQueue();
    for( int i = 0; i < pushCount; ++i )
      mInfoQueueDEBUG->PopStorageFilter();
  }

  void ScopedDXFilter::Push( D3D11_MESSAGE_ID msg ) { Push( &msg, 1 ); }

  void ScopedDXFilter::Push(  D3D11_MESSAGE_ID* msgs, int n )
  {
    D3D11_MESSAGE_ID* dst = denyList.end();
    denyList.append_range( msgs, n );

    const D3D11_INFO_QUEUE_FILTER_DESC DenyList =
    {
      .NumIDs = (UINT)n,
      .pIDList = dst,
    };

    D3D11_INFO_QUEUE_FILTER filter =
    {
      .DenyList = DenyList
    };

    ID3D11InfoQueue* mInfoQueueDEBUG = RendererDirectX11::GetInfoQueue();
    mInfoQueueDEBUG->PushStorageFilter( &filter );
    pushCount++;
  }

  void SetDebugNameAux( IDXGIObject* dxgiObj,
                                        const StringView& name,
                                        const StringView& suffix )
  {
    DXGINamer namer{ dxgiObj };
    SetTDebugName( &namer, name, suffix);
  }

  void SetDebugNameAux( ID3D11DeviceChild* directXObject,
                                        const StringView& name,
                                        const StringView& suffix )
  {
    ID3D11DeviceChildNamer namer{ directXObject };
    SetTDebugName( &namer, name, suffix);
  }


} // namespace Tac::Render

