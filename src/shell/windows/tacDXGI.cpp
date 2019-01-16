#include "tacDXGI.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacPreprocessor.h"

#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

#include <sstream> // std::stringstream

#pragma comment( lib, "DXGI.lib" )

void TacDXGI::Init( TacErrors& errors )
{

  UINT flags = TacIsDebugMode() ? DXGI_CREATE_FACTORY_DEBUG : 0;
  HRESULT  hr = CreateDXGIFactory2( flags, IID_PPV_ARGS( &mFactory ) );
  if( FAILED( hr ) )
  {
    errors = "failed to create dxgi factory";
    return;
  }
  TacNameDXGIObject( mFactory, "tac dxgi factory" );

  IDXGIAdapter1* dxgiAdapter1;
  SIZE_T maxDedicatedVideoMemory = 0;
  for( UINT i = 0; mFactory->EnumAdapters1( i, &dxgiAdapter1 ) != DXGI_ERROR_NOT_FOUND; ++i )
  {
    DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
    dxgiAdapter1->GetDesc1( &dxgiAdapterDesc1 );

    if( dxgiAdapterDesc1.DedicatedVideoMemory < maxDedicatedVideoMemory )
      continue;
    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
    TAC_DXGI_CALL( errors, dxgiAdapter1->QueryInterface, IID_PPV_ARGS( &mDxgiAdapter4 ) );
  }
  TacNameDXGIObject( mDxgiAdapter4, "tac dxgi adaptor" );
}

void TacDXGI::CreateSwapChain(
  HWND hwnd,
  IUnknown* pDevice,
  int bufferCount,
  UINT width,
  UINT height,
  IDXGISwapChain** ppSwapChain,
  TacErrors& errors )
{
  IDXGISwapChain1* swapChain;
  DXGI_SWAP_CHAIN_DESC1 scd1 = {};
  {
    scd1.Width = width;
    scd1.Height = height;
    scd1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    scd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd1.SampleDesc.Count = 1;
    scd1.BufferCount = bufferCount;
    scd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  }
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfsd = {};
  {
    scfsd.RefreshRate.Numerator = 1;
    scfsd.RefreshRate.Denominator = 60;
    scfsd.Windowed = TRUE;
    scfsd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scfsd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  }

  // This call deprecates IDXGIFactory::CreateSwapChain
  HRESULT hr = mFactory->CreateSwapChainForHwnd( pDevice, hwnd, &scd1, &scfsd, NULL, &swapChain );
  if( FAILED( hr ) )
  {
    errors = "Failed to create swap chain";
    return;
  }

  *ppSwapChain = swapChain;
}

TacVector< std::pair< TacFormat, DXGI_FORMAT > > GetFormatPairs()
{
  return {
    { { 2, sizeof( float ), TacGraphicsType::real }, DXGI_FORMAT_R32G32_FLOAT },
  { { 3, sizeof( float ), TacGraphicsType::real }, DXGI_FORMAT_R32G32B32_FLOAT },
  { { 4, sizeof( float ), TacGraphicsType::real }, DXGI_FORMAT_R32G32B32A32_FLOAT },
  { { 1, sizeof( uint8_t ), TacGraphicsType::unorm }, DXGI_FORMAT_R8_UNORM },
  { { 2, sizeof( uint8_t ), TacGraphicsType::unorm }, DXGI_FORMAT_R8G8_UNORM },
  { { 4, sizeof( uint8_t ), TacGraphicsType::unorm }, DXGI_FORMAT_R8G8B8A8_UNORM },
  { { 1, sizeof( uint16_t ), TacGraphicsType::uint }, DXGI_FORMAT_R16_UINT },
  };
}

TacFormat GetTacFormat( DXGI_FORMAT format )
{
  for( auto formatPair : GetFormatPairs() )
  {
    if( formatPair.second != format )
      continue;
    return formatPair.first;
  }
  TacInvalidCodePath;
  return {};
}

DXGI_FORMAT GetDXGIFormat( TacFormat textureFormat )
{
  for( auto formatPair : GetFormatPairs() )
  {
    auto curTacFormat = formatPair.first;
    if( curTacFormat.mPerElementByteCount != textureFormat.mPerElementByteCount ||
      curTacFormat.mElementCount != textureFormat.mElementCount ||
      curTacFormat.mPerElementDataType != textureFormat.mPerElementDataType )
      continue;
    return formatPair.second;
  }
  TacInvalidCodePath;
  return DXGI_FORMAT_UNKNOWN;
}


void TacNameDXGIObject( IDXGIObject* object, const TacString& name )
{
  // https://docs.microsoft.com/en-us/windows/desktop/api/dxgi/nf-dxgi-idxgiobject-setprivatedata
  HRESULT hr = object->SetPrivateData( WKPDID_D3DDebugObjectName, name.size(), name.data() );
  TacAssert( hr == S_OK );
}


static TacString TacTryInferDXGIErrorStr( HRESULT res )
{
  // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/dxgi-error
  switch( res )
  {
  case DXGI_ERROR_ACCESS_DENIED:
    return "DXGI_ERROR_ACCESS_DENIED You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
  case DXGI_ERROR_ACCESS_LOST:
    return "DXGI_ERROR_ACCESS_LOST The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
  case DXGI_ERROR_ALREADY_EXISTS:
    return "DXGI_ERROR_ALREADY_EXISTS The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
  case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
    return "DXGI_ERROR_CANNOT_PROTECT_CONTENT DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
  case DXGI_ERROR_DEVICE_HUNG:
    return "DXGI_ERROR_DEVICE_HUNG The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
  case DXGI_ERROR_DEVICE_REMOVED:
    return "DXGI_ERROR_DEVICE_REMOVED The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
  case DXGI_ERROR_DEVICE_RESET:
    return "DXGI_ERROR_DEVICE_RESET The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The driver encountered a problem and was put into the device removed state.";
  case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
    return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
  case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
  case DXGI_ERROR_INVALID_CALL:
    return "DXGI_ERROR_INVALID_CALL The application provided invalid parameter data; this must be debugged and fixed before the application is released.";
  case DXGI_ERROR_MORE_DATA:
    return "DXGI_ERROR_MORE_DATA The buffer supplied by the application is not big enough to hold the requested data.";
  case DXGI_ERROR_NAME_ALREADY_EXISTS:
    return "DXGI_ERROR_NAME_ALREADY_EXISTS The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
  case DXGI_ERROR_NONEXCLUSIVE:
    return "DXGI_ERROR_NONEXCLUSIVE A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
  case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
    return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE The resource or request is not currently available, but it might become available later.";
  case DXGI_ERROR_NOT_FOUND:
    return "DXGI_ERROR_NOT_FOUND When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
  case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
    return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED Reserved";
  case DXGI_ERROR_REMOTE_OUTOFMEMORY:
    return "DXGI_ERROR_REMOTE_OUTOFMEMORY Reserved";
  case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
    return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
  case DXGI_ERROR_SDK_COMPONENT_MISSING:
    return "DXGI_ERROR_SDK_COMPONENT_MISSING The operation depends on an SDK component that is missing or mismatched.";
  case DXGI_ERROR_SESSION_DISCONNECTED:
    return "DXGI_ERROR_SESSION_DISCONNECTED The Remote Desktop Services session is currently disconnected.";
  case DXGI_ERROR_UNSUPPORTED:
    return "DXGI_ERROR_UNSUPPORTED The requested functionality is not supported by the device or the driver.";
  case DXGI_ERROR_WAIT_TIMEOUT:
    return "DXGI_ERROR_WAIT_TIMEOUT The time - out interval elapsed before the next desktop frame was available.";
  case DXGI_ERROR_WAS_STILL_DRAWING:
    return "DXGI_ERROR_WAS_STILL_DRAWING The GPU was busy at the moment when a call was made to perform an operation, and did not execute or schedule the operation.";
  case S_OK:
    return "S_OK";
  default:
      return "";
  }
}
void TacDXGICallAux( const char* fnCallWithArgs, HRESULT res, TacErrors& errors )
{
  std::stringstream ss;
  ss << fnCallWithArgs << " returned 0x" << std::hex << res;
  TacString inferredErrorMessage = TacTryInferDXGIErrorStr( res );
  if(!inferredErrorMessage.empty())
  {
    ss << "(";
    ss << inferredErrorMessage;
    ss << ")";
  }
  errors.mMessage = ss.str().c_str();
}

