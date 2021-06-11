#include "src/common/containers/tacArray.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/tacTextParser.h"
#include "src/common/tacUtility.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/common/math/tacMath.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/shell/tacShell.h"
#include "src/common/tacOS.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/string/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacDXGI.h"
#include "src/shell/windows/tacRendererDirectX11.h"

#include <initguid.h>
#include <dxgidebug.h>
#include <D3DCompiler.h> // D3DCOMPILE_...
#include <d3dcommon.h> // WKPDID_D3DDebugObjectName

#include <iostream> // std::cout
#include <utility> // std::pair
#include <sstream> // std::stringstream



#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "D3DCompiler.lib" )


namespace Tac
{
  namespace Render
  {
    static bool gVerbose;

    static int registerDX11 = []()
    {
      RendererFactoriesRegister( { RendererNameDirectX11,
                                 []() { TAC_NEW RendererDirectX11;  } } );
      return 0;
    }( );


#define TAC_DX11_CALL( errors, call, ... )                                                       \
{                                                                                                \
  const HRESULT result = call( __VA_ARGS__ );                                                    \
  if( FAILED( result ) )                                                                         \
  {                                                                                              \
    const String errorMsg = DX11CallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result ); \
    TAC_RAISE_ERROR( errorMsg, errors );                                                         \
  }                                                                                              \
}

#define TAC_DX11_CALL_RETURN( errors, retval, call, ... )                                                       \
{                                                                                                \
  const HRESULT result = call( __VA_ARGS__ );                                                    \
  if( FAILED( result ) )                                                                         \
  {                                                                                              \
    const String errorMsg = DX11CallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result ); \
    TAC_RAISE_ERROR_RETURN( errorMsg, errors, retval );                                                         \
  }                                                                                              \
}


    static String GetDirectX11ShaderPath( StringView shaderName )
    {
      String result;
      const char* prefix = "assets/hlsl/";
      const char* suffix = ".fx";
      result += shaderName.starts_with( prefix ) ? "" : prefix;
      result += shaderName;
      result += shaderName.ends_with( suffix ) ? "" : suffix;
      return result;
    }

    static String TryInferDX11ErrorStr( HRESULT res )
    {
      switch( res )
      {
        case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND	The file was not found.";
        case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS	There are too many unique instances of a particular type of state object.";
        case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS	There are too many unique instances of a particular type of view object.";
        case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD	The first call to ID3D11DeviceContext::Map after either ID3D11Device::CreateDeferredContext or ID3D11DeviceContext::FinishCommandList per Resource was not D3D11_MAP_WRITE_DISCARD.";

          // move to Dxgi.cpp?
        case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL The method call is invalid.For example, a method's parameter may not be a valid pointer.";
        case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING The previous blit operation that is transferring information to or from this surface is incomplete.";
        case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED You tried to use a resource to which you did not have the required access privileges.This error is most typically caused when you write to a shared resource with read - only access.";
        case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST The desktop duplication interface is invalid.The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";
        case DXGI_ERROR_ALREADY_EXISTS: return "DXGI_ERROR_ALREADY_EXISTS The desired element already exists.This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";
        case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";
        case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";
        case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED The video card has been physically removed from the system, or a driver upgrade for the video card has occurred.The application should destroy and recreate the device.For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";
        case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET The device failed due to a badly formed command.This is a run - time issue; The application should destroy and recreate the device.";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR The driver encountered a problem and was put into the device removed state.";
        case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT An event( for example, a power cycle ) interrupted the gathering of presentation statistics.";
        case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE The application attempted to acquire exclusive ownership of an output, but failed because some other application( or device within the application ) already acquired ownership.";
        case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA The buffer supplied by the application is not big enough to hold the requested data.";
        case DXGI_ERROR_NAME_ALREADY_EXISTS: return "DXGI_ERROR_NAME_ALREADY_EXISTS The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";
        case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE A global counter resource is in use, and the Direct3D device can't currently use the counter resource.";
        case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE The resource or request is not currently available, but it might become available later.";
        case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface.When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";
        case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED Reserved";
        case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY Reserved";
        case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE The DXGI output( monitor ) to which the swap chain content was restricted is now disconnected or changed.";
        case DXGI_ERROR_SDK_COMPONENT_MISSING: return "DXGI_ERROR_SDK_COMPONENT_MISSING The operation depends on an SDK component that is missing or mismatched.";
        case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED The Remote Desktop Services session is currently disconnected.";
        case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED The requested functionality is not supported by the device or the driver.";
        case DXGI_ERROR_WAIT_TIMEOUT: return "DXGI_ERROR_WAIT_TIMEOUT The time - out interval elapsed before the next desktop frame was available.";

        case E_FAIL: return "E_FAIL	Attempted to create a device with the debug layer enabled and the layer is not installed.";
        case E_INVALIDARG: return "E_INVALIDARG	An invalid parameter was passed to the returning function.";
        case E_OUTOFMEMORY: return "E_OUTOFMEMORY	Direct3D could not allocate sufficient memory to complete the call.";
        case E_NOTIMPL: return "E_NOTIMPL	The method call isn't implemented with the passed parameter combination.";
        case S_FALSE: return "S_FALSE	Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
        case S_OK: return "S_OK	No error occurred.";
        default: return "idk lol";
      }
    }

    static String DX11CallAux( const char* fnCallWithArgs, HRESULT res )
    {
      std::stringstream ss;
      ss << fnCallWithArgs << " returned 0x" << std::hex << res;
      String inferredErrorMessage = TryInferDX11ErrorStr( res );
      if( !inferredErrorMessage.empty() )
      {
        ss << "(";
        ss << inferredErrorMessage.c_str();
        ss << ")";
      }
      return  ss.str().c_str();
    }

    static void ReportLiveObjects()
    {
      if( !IsDebugMode() )
        return;
      auto Dxgidebughandle = GetModuleHandle( "Dxgidebug.dll" );
      if( !Dxgidebughandle )
        return;

      auto myDXGIGetDebugInterface =
        ( HRESULT( WINAPI* )( REFIID, void** ) )GetProcAddress(
          Dxgidebughandle,
          "DXGIGetDebugInterface" );

      if( !myDXGIGetDebugInterface )
        return;
      IDXGIDebug* myIDXGIDebug;
      HRESULT hr = myDXGIGetDebugInterface( IID_PPV_ARGS( &myIDXGIDebug ) );
      if( FAILED( hr ) )
        return;
      myIDXGIDebug->ReportLiveObjects(
        DXGI_DEBUG_ALL,
        DXGI_DEBUG_RLO_ALL );
      myIDXGIDebug->Release();
    }

    static D3D11_TEXTURE_ADDRESS_MODE GetAddressMode( AddressMode addressMode )
    {
      switch( addressMode )
      {
        case AddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
        case AddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
        case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( addressMode ); return D3D11_TEXTURE_ADDRESS_WRAP;
      }
    }

    static D3D11_COMPARISON_FUNC GetCompare( Comparison compare )
    {
      switch( compare )
      {
        case Comparison::Always: return D3D11_COMPARISON_ALWAYS;
        case Comparison::Never: return D3D11_COMPARISON_NEVER;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( compare ); return D3D11_COMPARISON_ALWAYS;
      }
    };

    static D3D11_FILTER GetFilter( Filter filter )
    {
      switch( filter )
      {
        case Filter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        case Filter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
        case Filter::Aniso: return D3D11_FILTER_ANISOTROPIC;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( filter ); return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      }
    };

    static D3D11_COMPARISON_FUNC GetDepthFunc( DepthFunc depthFunc )
    {
      switch( depthFunc )
      {
        case DepthFunc::Less: return D3D11_COMPARISON_LESS;
        case DepthFunc::LessOrEqual: return D3D11_COMPARISON_LESS_EQUAL;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( depthFunc ); return D3D11_COMPARISON_LESS;
      }
    }

    static D3D11_USAGE GetUsage( Access access )
    {
      switch( access )
      {
        case Access::Default: return D3D11_USAGE_DEFAULT;
        case Access::Dynamic: return D3D11_USAGE_DYNAMIC;
        case Access::Static: return D3D11_USAGE_IMMUTABLE;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( access ); return D3D11_USAGE_DEFAULT;
      }
    }

    static UINT GetCPUAccessFlags( CPUAccess access )
    {
      UINT result = 0;
      if( ( int )access & ( int )CPUAccess::Read )
        result |= D3D11_CPU_ACCESS_READ;
      if( ( int )access & ( int )CPUAccess::Write )
        result |= D3D11_CPU_ACCESS_WRITE;
      return result;
    }

    static UINT GetBindFlags( Binding binding )
    {
      UINT BindFlags = 0;
      if( ( int )binding & ( int )Binding::RenderTarget )
        BindFlags |= D3D11_BIND_RENDER_TARGET;
      if( ( int )binding & ( int )Binding::ShaderResource )
        BindFlags |= D3D11_BIND_SHADER_RESOURCE;
      if( ( int )binding & ( int )Binding::DepthStencil )
        BindFlags |= D3D11_BIND_DEPTH_STENCIL;
      if( ( int )binding & ( int )Binding::UnorderedAccess )
        BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
      return BindFlags;
    }

    static UINT GetMiscFlags( Binding binding )
    {
      if( ( int )binding & ( int )Binding::RenderTarget &&
        ( int )binding & ( int )Binding::ShaderResource )
        return D3D11_RESOURCE_MISC_GENERATE_MIPS;
      return 0;
    }

    static D3D11_BLEND GetBlend( BlendConstants c )
    {
      switch( c )
      {
        case BlendConstants::OneMinusSrcA:
          return D3D11_BLEND_INV_SRC_ALPHA;
        case BlendConstants::SrcA:
          return D3D11_BLEND_SRC_ALPHA;
        case BlendConstants::SrcRGB:
          return D3D11_BLEND_SRC_COLOR;
        case BlendConstants::Zero:
          return D3D11_BLEND_ZERO;
        case BlendConstants::One:
          return D3D11_BLEND_ONE;
        default:
          TAC_ASSERT( false );
          return D3D11_BLEND_ZERO;
      }
    };

    static D3D11_BLEND_OP GetBlendOp( BlendMode mode )
    {
      switch( mode )
      {
        case BlendMode::Add:
          return D3D11_BLEND_OP_ADD;
        default:
          TAC_ASSERT( false );
          return D3D11_BLEND_OP_ADD;
      }
    };

    static D3D11_FILL_MODE GetFillMode( FillMode fillMode )
    {
      switch( fillMode )
      {
        case FillMode::Solid: return D3D11_FILL_SOLID;
        case FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( fillMode ); return ( D3D11_FILL_MODE )0;
      }
    }

    static D3D11_CULL_MODE GetCullMode( CullMode cullMode )
    {
      switch( cullMode )
      {
        case CullMode::None: return D3D11_CULL_NONE;
        case CullMode::Back: return D3D11_CULL_BACK;
        case CullMode::Front: return D3D11_CULL_FRONT;
        default: TAC_CRITICAL_ERROR_INVALID_CASE( cullMode ); return ( D3D11_CULL_MODE )0;
      }
    }

    static WCHAR* ToTransientWchar( StringView str )
    {
      WCHAR* result = ( WCHAR* )FrameMemoryAllocate( ( sizeof( WCHAR ) + 1 ) * str.size() );
      WCHAR* resultIter = result;
      for( char c : str )
        *resultIter++ = ( WCHAR )c;
      *resultIter++ = 0;
      return result;
    }

    static String ShaderPathToContentString( StringView path, Errors& errors )
    {
      if( !path )
        return "";
      String shaderFilePath = GetDirectX11ShaderPath( path );
      String shaderFileContents = FileToString( shaderFilePath, errors );
      return shaderFileContents + "\n";
    }

    static ID3DBlob* CompileShaderFromString( const ShaderSource& shaderSource,
                                              const StringView shaderStr,
                                              const char* entryPoint,
                                              const char* shaderModel,
                                              Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
      dwShaderFlags |= IsDebugMode() ? D3DCOMPILE_DEBUG : 0;
      dwShaderFlags |= IsDebugMode() ? D3DCOMPILE_SKIP_OPTIMIZATION : 0;

      ID3DBlob* pErrorBlob = nullptr;
      ID3DBlob* pBlobOut = nullptr;
      const HRESULT hr = D3DCompile( shaderStr.data(),
                                     shaderStr.size(),
                                     nullptr,
                                     nullptr, // D3D_SHADER_MACRO* pDefines,
                                     nullptr, // ID3DInclude* pInclude,
                                     entryPoint,
                                     shaderModel,
                                     dwShaderFlags,
                                     0,
                                     &pBlobOut,
                                     &pErrorBlob );
      if( FAILED( hr ) )
      {
        if( IsDebugMode() )
        {
          std::cout << "Error loading shader from ";
          if( shaderSource.mType == Render::ShaderSource::Type::kPath )
            std::cout << "path: " << GetDirectX11ShaderPath( shaderSource.mStr ) << std::endl;
          else if( shaderSource.mType == Render::ShaderSource::Type::kStr )
            std::cout << "string";

          const char* shaderBlock = "----------------";
          std::cout << shaderBlock << std::endl;
          int lineNumber = 0;
          bool isNewLine = true;
          for( char c : shaderStr )
          {
            if( isNewLine )
            {
              std::cout << va( "line %3i|", ++lineNumber );
              isNewLine = false;
            }
            std::cout << c;
            isNewLine = c == '\n';
          }
          std::cout << std::endl;
          std::cout << shaderBlock << std::endl;

        }

        const char* errMsg = ( const char* )pErrorBlob->GetBufferPointer();
        TAC_RAISE_ERROR_RETURN( errMsg, errors, nullptr );
      }

      return pBlobOut;
    }

    static String InlineShaderIncludes( String shaderSourceCode, Errors& errors )
    {
      String result;
      ParseData shaderParseData( shaderSourceCode.data(), shaderSourceCode.size() );
      for( ;; )
      {
        String line = shaderParseData.EatRestOfLine();
        if( line.empty() )
          break;

        ParseData lineParseData( line.data(), line.size() );
        lineParseData.EatWhitespace();
        if( lineParseData.EatStringExpected( "#include" ) )
        {
          lineParseData.EatUntilCharIsPrev( '\"' );
          const char*      includeBegin = lineParseData.GetPos();
          lineParseData.EatUntilCharIsNext( '\"' );
          const char*      includeEnd = lineParseData.GetPos();
          const StringView includeName( includeBegin, includeEnd );
          const String     includePath = GetDirectX11ShaderPath( includeName );
          const String     includeSource = ShaderPathToContentString( includePath, errors );
          const String     includeSourceInlined = InlineShaderIncludes( includeSource, errors );

          line = "";
          line += "//===----- (begin include " + includePath + ") -----===//\n";
          line += includeSource;
          line += "//===----- (end include " + includePath + ") -----===//\n";
        }

        result += line;
      }

      return result;
    }

    static bool DoesShaderTextContainEntryPoint( StringView shaderText, const char* entryPoint )
    {
      const char* searchableEntryPoint = FrameMemoryPrintf( "%s(", entryPoint );
      return shaderText.find( searchableEntryPoint ) != StringView::npos;
    }

    static Program LoadProgram( Render::ShaderSource shaderSource, Errors& errors )
    {
      // Errors2 can debug break on append, errors will not
      //Errors errors;
      //TAC_ON_DESTRUCT( errors2 = errors );

      ID3D11Device* device = ( ( RendererDirectX11* )Renderer::Instance )->mDevice;
      ID3D11VertexShader*   vertexShader = nullptr;
      ID3D11PixelShader*    pixelShader = nullptr;
      ID3D11GeometryShader* geometryShader = nullptr;
      ID3DBlob* inputSignature = nullptr;

      const String shaderPath
        = shaderSource.mType == Render::ShaderSource::Type::kPath
        ? GetDirectX11ShaderPath( shaderSource.mStr )
        : "<inlined  shader>";

      for( ;; )
      {
        if( errors )
        {
          if( IsDebugMode() )
          {
            if( shaderSource.mType == Render::ShaderSource::Type::kPath )
              errors.Append( "Error compiling shader: " + shaderPath );
            errors.Append( TAC_STACK_FRAME );
            OSDebugPopupBox( errors.ToString() );
            errors.clear();
          }
          else
          {
            TAC_HANDLE_ERROR_RETURN( errors, Program() );
          }
        }

        String shaderStringFull;
        switch( shaderSource.mType )
        {
          case Render::ShaderSource::Type::kPath:
            shaderStringFull += ShaderPathToContentString( shaderSource.mStr, errors );
            break;
          case Render::ShaderSource::Type::kStr:
            shaderStringFull += shaderSource.mStr;
            break;
        }
        if( errors )
          continue;

        shaderStringFull = InlineShaderIncludes( shaderStringFull, errors );
        if( errors )
          continue;

        const char* vertexShaderEntryPoint = "VS";
        const char* pixelShaderEntryPoint = "PS";
        const char* geometryShaderEntryPoint = "GS";
        const bool hasVertexShader = DoesShaderTextContainEntryPoint( shaderStringFull, vertexShaderEntryPoint );
        const bool hasGeometryShader = DoesShaderTextContainEntryPoint( shaderStringFull, geometryShaderEntryPoint );
        const bool hasPixelShader = DoesShaderTextContainEntryPoint( shaderStringFull, pixelShaderEntryPoint );
        TAC_ASSERT_MSG( hasVertexShader, "shader %s missing %s", shaderPath.c_str(), vertexShaderEntryPoint );
        TAC_ASSERT_MSG( hasPixelShader, "shader %s missing %s", shaderPath.c_str(), pixelShaderEntryPoint );

        auto GetShaderModel = []( const char* prefix ) { return FrameMemoryPrintf( "%s_5_0", prefix ); };

        if( hasVertexShader )
        {
          ID3DBlob* pVSBlob = CompileShaderFromString( shaderSource,
                                                       shaderStringFull,
                                                       vertexShaderEntryPoint,
                                                       GetShaderModel( "vs" ),
                                                       errors );
          if( errors )
            continue;
          TAC_ON_DESTRUCT( pVSBlob->Release() );
          TAC_DX11_CALL_RETURN( errors, Program(),
                                device->CreateVertexShader,
                                pVSBlob->GetBufferPointer(),
                                pVSBlob->GetBufferSize(),
                                nullptr,
                                &vertexShader );
          TAC_DX11_CALL_RETURN( errors, Program(),
                                D3DGetBlobPart,
                                pVSBlob->GetBufferPointer(),
                                pVSBlob->GetBufferSize(),
                                D3D_BLOB_INPUT_SIGNATURE_BLOB,
                                0,
                                &inputSignature );
        }


        if( hasPixelShader )
        {
          ID3DBlob* pPSBlob = CompileShaderFromString( shaderSource,
                                                       shaderStringFull,
                                                       pixelShaderEntryPoint,
                                                       GetShaderModel( "ps" ),
                                                       errors );
          if( errors )
            continue;
          TAC_ON_DESTRUCT( pPSBlob->Release() );

          TAC_DX11_CALL_RETURN( errors, Program(),
                                device->CreatePixelShader,
                                pPSBlob->GetBufferPointer(),
                                pPSBlob->GetBufferSize(),
                                nullptr,
                                &pixelShader );
          if( errors )
            continue;
        }

        if( hasGeometryShader )
        {
          ID3DBlob* blob = CompileShaderFromString( shaderSource,
                                                    shaderStringFull,
                                                    geometryShaderEntryPoint,
                                                    GetShaderModel( "gs" ),
                                                    errors );
          if( errors )
            continue;
          TAC_ON_DESTRUCT( blob->Release() );

          TAC_DX11_CALL_RETURN( errors, Program(),
                                device->CreateGeometryShader,
                                blob->GetBufferPointer(),
                                blob->GetBufferSize(),
                                nullptr,
                                &geometryShader );
          if( errors )
            continue;
        }

        break;
      }

      Program program;
      program.mInputSig = inputSignature;
      program.mVertexShader = vertexShader;
      program.mPixelShader = pixelShader;
      program.mGeometryShader = geometryShader;
      return program;
    }

    RendererDirectX11::~RendererDirectX11()
    {
      DXGIUninit();
      //mDxgi.Uninit();
      if( mDeviceContext )
      {
        mDeviceContext->Release();
        mDeviceContext = nullptr;
      }
      if( mDevice )
      {
        mDevice->Release();
        mDevice = nullptr;
      }
      if( mInfoQueueDEBUG )
      {
        mInfoQueueDEBUG->Release();
        mInfoQueueDEBUG = nullptr;
      }
      if( mUserAnnotationDEBUG )
      {
        mUserAnnotationDEBUG->Release();
        mUserAnnotationDEBUG = nullptr;
      }
      ReportLiveObjects();
    }

    void RendererDirectX11::Init( Errors& errors )
    {
      mName = RendererNameDirectX11;
      UINT createDeviceFlags = 0;
      if( IsDebugMode() )
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

      D3D_FEATURE_LEVEL featureLevel;
      D3D_FEATURE_LEVEL featureLevels[ 10 ];
      int featureLevelCount = 0;
      featureLevels[ featureLevelCount++ ] = D3D_FEATURE_LEVEL_11_0;

      IDXGIAdapter* pAdapter = NULL;
      D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_HARDWARE;
      HMODULE Software = NULL;

      TAC_DX11_CALL( errors,
                     D3D11CreateDevice,
                     pAdapter,
                     DriverType,
                     Software,
                     createDeviceFlags,
                     featureLevels,
                     featureLevelCount,
                     D3D11_SDK_VERSION,
                     &mDevice,
                     &featureLevel,
                     &mDeviceContext );
      // If you're directx is crashing / throwing exception, don't forget to check
      // your output window, it likes to put error messages there
      if( IsDebugMode() )
      {
        TAC_DX11_CALL( errors, mDevice->QueryInterface, IID_PPV_ARGS( &mInfoQueueDEBUG ) );
        TAC_DX11_CALL( errors, mDeviceContext->QueryInterface, IID_PPV_ARGS( &mUserAnnotationDEBUG ) );
      }

      DXGIInit( errors );
      //mDxgi.Init( errors );
      TAC_HANDLE_ERROR( errors );

    }

    void RendererDirectX11::RenderBegin( const Render::Frame*, Errors& )
    {
      if( gVerbose )
        std::cout << "Render2::Begin\n";

      for( auto& b : mBoundConstantBuffers )
        b = {};
      mBoundConstantBufferCount = 0;

//       for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
//       {
//         const Render::FramebufferHandle framebufferHandle = mWindows[ iWindow ];
//         TAC_ASSERT( framebufferHandle.IsValid() );
// 
//         Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
//         ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
//         ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;
// 
//         const UINT ClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
//         const FLOAT ClearDepth = 1.0f;
//         const UINT8 ClearStencil = 0;
//         mDeviceContext->ClearDepthStencilView( depthStencilView, ClearFlags, ClearDepth, ClearStencil );
// 
//         const FLOAT ClearGrey = 0.5f;
//         const FLOAT ClearColorRGBA[] = { ClearGrey, ClearGrey, ClearGrey,  1.0f };
//         mDeviceContext->ClearRenderTargetView( renderTargetView, ClearColorRGBA );
//       }
      for( int i = 0; i < kMaxFramebuffers; ++i )
        mFramebuffersBoundEverThisFrame[ i ] = false;

      mBlendState = nullptr;
      mDepthStencilState = nullptr;
      mViewHandle = Render::ViewHandle();
      mIndexBuffer = nullptr;

    }

    void RendererDirectX11::RenderEnd( const Render::Frame*, Errors& )
    {
      Render::ShaderReloadHelperUpdate(
        []( Render::ShaderHandle shaderHandle, const char* path )
        {
          RendererDirectX11* renderer = ( RendererDirectX11* )Renderer::Instance;
          Errors errors;
          renderer->mPrograms[ ( int )shaderHandle ] = LoadProgram( Render::ShaderSource::FromPath( path ), errors );
        } );

      if( gVerbose )
        std::cout << "Render2::End\n";
    }

#if 0
    static void WaitUntilDrawCallFinishes( ID3D11Device* device, ID3D11DeviceContext* deviceContext )
    {
      D3D11_QUERY_DESC desc = {};
      desc.Query = D3D11_QUERY_EVENT;
      ID3D11Query* query;
      auto createQueryResult = device->CreateQuery( &desc, &query );
      TAC_ASSERT( SUCCEEDED( createQueryResult ) );
      deviceContext->End( query );

      for( ;; )
      {
        HRESULT getDataResult = deviceContext->GetData( query, nullptr, 0, 0 );
        if( getDataResult == S_FALSE )
        {
          // not finished gpu commands executing
        }
        else if( getDataResult == S_OK )
        {
          break;
        }
        else
        {
          TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
        }
      }

      query->Release();
    }
#endif

    void RendererDirectX11::RenderDrawCall( const Render::Frame* frame,
                                            const Render::DrawCall3* drawCall,
                                            Errors& errors )
    {


      if( drawCall->mShaderHandle.IsValid() )
      {
        Program* program = &mPrograms[ ( int )drawCall->mShaderHandle ];
        TAC_ASSERT( program->mVertexShader );
        TAC_ASSERT( program->mPixelShader );
        // Bind new shaders / unbind unused shaders
        mDeviceContext->VSSetShader( program->mVertexShader, NULL, 0 );
        mDeviceContext->PSSetShader( program->mPixelShader, NULL, 0 );
        mDeviceContext->GSSetShader( program->mGeometryShader, NULL, 0 );
      }

      if( drawCall->mBlendStateHandle.IsValid() && mBlendState != mBlendStates[ ( int )drawCall->mBlendStateHandle ] )
      {
        mBlendState = mBlendStates[ ( int )drawCall->mBlendStateHandle ];
        TAC_ASSERT( mBlendState );
        const FLOAT blendFactorRGBA[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        const UINT sampleMask = 0xffffffff;
        mDeviceContext->OMSetBlendState( mBlendState, blendFactorRGBA, sampleMask );
      }

      if( drawCall->mDepthStateHandle.IsValid() && mDepthStencilState != mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ] )
      {
        mDepthStencilState = mDepthStencilStates[ ( int )drawCall->mDepthStateHandle ];
        TAC_ASSERT( mDepthStencilState );
        const UINT stencilRef = 0;
        mDeviceContext->OMSetDepthStencilState( mDepthStencilState, stencilRef );
      }

      if( drawCall->mIndexBufferHandle.IsValid() )
      {
        mIndexBuffer = &mIndexBuffers[ ( int )drawCall->mIndexBufferHandle ];
        if( !mIndexBuffer->mBuffer )
          OSDebugBreak();
        TAC_ASSERT( mIndexBuffer->mBuffer );
        const DXGI_FORMAT dxgiFormat = GetDXGIFormat( mIndexBuffer->mFormat );
        const UINT byteOffset = 0; //  drawCall->mStartIndex * indexBuffer->mFormat.mPerElementByteCount;
        mDeviceContext->IASetIndexBuffer( mIndexBuffer->mBuffer,
                                          dxgiFormat,
                                          byteOffset );
      }

      if( drawCall->mVertexBufferHandle.IsValid() )
      {
        const VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )drawCall->mVertexBufferHandle ];
        TAC_ASSERT( vertexBuffer->mBuffer );
        const UINT startSlot = 0;
        const UINT NumBuffers = 1;
        const UINT Strides[ NumBuffers ] = { ( UINT )vertexBuffer->mStride };
        const UINT ByteOffsets[ NumBuffers ] =
        {
          0
          // ( UINT )( drawCall->mStartVertex * vertexBuffer->mStride )
        };
        ID3D11Buffer* buffers[ NumBuffers ] = { vertexBuffer->mBuffer };
        mDeviceContext->IASetVertexBuffers( startSlot,
                                            NumBuffers,
                                            buffers,
                                            Strides,
                                            ByteOffsets );
      }

      if( drawCall->mRasterizerStateHandle.IsValid() )
      {
        ID3D11RasterizerState* rasterizerState = mRasterizerStates[ ( int )drawCall->mRasterizerStateHandle ];
        TAC_ASSERT( rasterizerState );
        mDeviceContext->RSSetState( rasterizerState );
      }

      if( drawCall->mSamplerStateHandle.IsValid() )
      {
        const UINT StartSlot = 0;
        const UINT NumSamplers = 1;
        ID3D11SamplerState* samplerState = mSamplerStates[ ( int )drawCall->mSamplerStateHandle ];
        TAC_ASSERT( samplerState );
        ID3D11SamplerState* Samplers[] = { samplerState };
        mDeviceContext->VSSetSamplers( StartSlot, NumSamplers, Samplers );
        mDeviceContext->PSSetSamplers( StartSlot, NumSamplers, Samplers );
      }

      if( drawCall->mVertexFormatHandle.IsValid() )
      {
        ID3D11InputLayout* inputLayout = mInputLayouts[ ( int )drawCall->mVertexFormatHandle ];
        TAC_ASSERT( inputLayout );
        mDeviceContext->IASetInputLayout( inputLayout );
      }

      if( drawCall->mViewHandle.IsValid() &&
          drawCall->mViewHandle != mViewHandle )
      {
        mViewHandle = drawCall->mViewHandle;
        const Render::View* view = &frame->mViews[ ( int )mViewHandle ];

        TAC_ASSERT_MSG( view->mFrameBufferHandle.IsValid(), "Did you forget to call Render::SetViewFramebuffer" );

        Framebuffer* framebuffer = &mFramebuffers[ ( int )view->mFrameBufferHandle ];
        if( !mFramebuffersBoundEverThisFrame[ ( int )view->mFrameBufferHandle ] )
        {
          mFramebuffersBoundEverThisFrame[ ( int )view->mFrameBufferHandle ] = true;


          ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
          ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;

          const UINT ClearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
          const FLOAT ClearDepth = 1.0f;
          const UINT8 ClearStencil = 0;
          mDeviceContext->ClearDepthStencilView( depthStencilView, ClearFlags, ClearDepth, ClearStencil );

          const FLOAT ClearGrey = 0.5f;
          const FLOAT ClearColorRGBA[] = { ClearGrey, ClearGrey, ClearGrey,  1.0f };
          mDeviceContext->ClearRenderTargetView( renderTargetView, ClearColorRGBA );


          ID3D11ShaderResourceView* nullViews[ 16 ] = {};
          mDeviceContext->VSSetShaderResources( 0, 16, nullViews );
          mDeviceContext->PSSetShaderResources( 0, 16, nullViews );
          mDeviceContext->GSSetShaderResources( 0, 16, nullViews );
        }


        ID3D11RenderTargetView* renderTargetView = framebuffer->mRenderTargetView;
        ID3D11DepthStencilView* depthStencilView = framebuffer->mDepthStencilView;
        TAC_ASSERT( renderTargetView );
        TAC_ASSERT( depthStencilView );
        UINT NumViews = 1;
        ID3D11RenderTargetView* RenderTargetViews[] = { renderTargetView };
        mDeviceContext->OMSetRenderTargets( NumViews, RenderTargetViews, depthStencilView );

        TAC_ASSERT( view->mViewportSet );
        D3D11_VIEWPORT viewport;
        viewport.Height = view->mViewport.mHeight;
        viewport.Width = view->mViewport.mWidth;
        viewport.TopLeftX = view->mViewport.mBottomLeftX;
        viewport.TopLeftY = -view->mViewport.mBottomLeftY; // convert opengl to directx
        viewport.MinDepth = view->mViewport.mMinDepth;
        viewport.MaxDepth = view->mViewport.mMaxDepth;
        mDeviceContext->RSSetViewports( 1, &viewport );

        // used if the rasterizer state ScissorEnable is TRUE
        TAC_ASSERT( view->mScissorSet );
        D3D11_RECT scissor;
        scissor.bottom = ( LONG )view->mScissorRect.mYMaxRelUpperLeftCornerPixel;
        scissor.left = ( LONG )view->mScissorRect.mXMinRelUpperLeftCornerPixel;
        scissor.right = ( LONG )view->mScissorRect.mXMaxRelUpperLeftCornerPixel;
        scissor.top = ( LONG )view->mScissorRect.mYMinRelUpperLeftCornerPixel;
        mDeviceContext->RSSetScissorRects( 1, &scissor );
      }

      if( drawCall->mTextureHandle.size() )
      {
        const UINT StartSlot = 0;
        const UINT NumViews = drawCall->mTextureHandle.size();
        ID3D11ShaderResourceView* ShaderResourceViews[ D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT ] = {};
        for( int iSlot = 0; iSlot < drawCall->mTextureHandle.size(); ++iSlot )
        {
          const Render::TextureHandle textureHandle = drawCall->mTextureHandle[ iSlot ];
          if( !textureHandle.IsValid() )
            continue;
          const Texture* texture = &mTextures[ ( int )textureHandle ];
          TAC_ASSERT( texture->mTextureSRV ); // Did you set the Render::TexSpec::mBinding?
          ShaderResourceViews[ iSlot ] = texture->mTextureSRV;
        }

        mDeviceContext->VSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
        mDeviceContext->PSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
        mDeviceContext->GSSetShaderResources( StartSlot, NumViews, ShaderResourceViews );
      }

      for( const Render::UpdateConstantBufferData& stuff : drawCall->mUpdateConstantBuffers )
      {
        const ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )stuff.mConstantBufferHandle ];
        TAC_ASSERT( constantBuffer->mBuffer );
        UpdateBuffer( constantBuffer->mBuffer,
                      stuff.mBytes,
                      stuff.mByteCount,
                      errors );

        mBoundConstantBuffers[ constantBuffer->mShaderRegister ] = constantBuffer->mBuffer;
        mBoundConstantBufferCount = Max( mBoundConstantBufferCount, constantBuffer->mShaderRegister + 1 );

        const UINT StartSlot = 0;
        mDeviceContext->PSSetConstantBuffers( StartSlot, mBoundConstantBufferCount, mBoundConstantBuffers );
        mDeviceContext->VSSetConstantBuffers( StartSlot, mBoundConstantBufferCount, mBoundConstantBuffers );
        mDeviceContext->GSSetConstantBuffers( StartSlot, mBoundConstantBufferCount, mBoundConstantBuffers );
      }

      mDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

      // not quite convinced this assert should be here.
      // on one hand, it prevents u from forgetting to set index count. ( good )
      // on the other, it prevents u from setting start index/index count
      //   seperately from setting the index buffer ( bad? )
      TAC_ASSERT( !drawCall->mIndexBufferHandle.IsValid() || drawCall->mIndexCount );

      if( drawCall->mIndexCount )
      {
        const UINT IndexCount = drawCall->mIndexCount;
        const UINT StartIndexLocation = drawCall->mStartIndex; //  *indexBuffer->mFormat.CalculateTotalByteCount();
        // should this be multiplied by the sizeof vertex?
        const INT BaseVertexLocation = 0; // drawCall->mStartVertex;

        mDeviceContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
      }
      else if( drawCall->mVertexCount )
      {
        const UINT VertexCount = drawCall->mVertexCount;
        const UINT StartVertexLocation = drawCall->mStartVertex;
        mDeviceContext->Draw( VertexCount, StartVertexLocation );
      }
    }

    void RendererDirectX11::SwapBuffers()
    {
      if( gVerbose )
        std::cout << "SwapBuffers::Begin\n";
      for( int iWindow = 0; iWindow < mWindowCount; ++iWindow )
        //for( int iFramebuffer = 0; iFramebuffer < Render::kMaxFramebuffers; ++iFramebuffer )
      {
        Render::FramebufferHandle framebufferHandle = mWindows[ iWindow ];
        Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
        if( !framebuffer->mSwapChain )
          continue;
        const UINT SyncInterval = 0;
        const UINT Flags = 0;
        framebuffer->mSwapChain->Present( SyncInterval, Flags );
      }
      if( gVerbose )
        std::cout << "SwapBuffers::End\n";
    }


    // Q: Should this function just return the clip space dimensions instead of A, B?
    void RendererDirectX11::GetPerspectiveProjectionAB( float f,
                                                        float n,
                                                        float& a,
                                                        float& b )
    {
      TAC_ASSERT( f > n );

      float invDenom = 1.0f / ( n - f );

      // ( A, B ) maps ( -n, -f ) to ( 0, 1 )
      // because clip space in directx is [ -1, 1 ][ -1, 1 ][ 0, 1 ]
      // note that clip space in opengl is [ -1, 1 ][ -1, 1 ][ -1, 1 ]
      // todo: double check this function
      a = f * invDenom;
      b = f * invDenom * n;
    }


    void RendererDirectX11::SetDebugName( ID3D11DeviceChild* directXObject,
                                          StringView name )
    {
      TAC_ASSERT( IsMainThread() );
      TAC_ASSERT( name.size() );
      if( !IsDebugMode() )
        return;
      TAC_ASSERT( directXObject );
      const int buffersize = 256;
      char data[ buffersize ] = {};

      UINT pDataSize = buffersize;
      directXObject->GetPrivateData( WKPDID_D3DDebugObjectName,
                                     &pDataSize,
                                     &data );

      String newname;
      if( pDataSize )
      {
        newname += String( data, pDataSize );
        newname += " --and-- ";

        D3D11_MESSAGE_ID hide[ 10 ];
        int hideCount = 0;
        hide[ hideCount++ ] = D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS;
        D3D11_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = hideCount;
        filter.DenyList.pIDList = hide;
        mInfoQueueDEBUG->PushStorageFilter( &filter );
      }
      newname += String( name );

      directXObject->SetPrivateData(
        WKPDID_D3DDebugObjectName,
        ( UINT )newname.size(),
        newname.c_str() );
      if( pDataSize )
      {
        mInfoQueueDEBUG->PopStorageFilter();
      }
    }



    void RendererDirectX11::AddMagicBuffer( Render::CommandDataCreateMagicBuffer* commandDataCreateMagicBuffer,
                                            Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      D3D11_BUFFER_DESC desc = {};
      desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
      desc.ByteWidth = commandDataCreateMagicBuffer->mByteCount;
      desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE
      desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      desc.StructureByteStride = commandDataCreateMagicBuffer->mStride;
      desc.Usage = D3D11_USAGE_DEFAULT; //  D3D11_USAGE_DYNAMIC; // GetUsage()

      MagicBuffer* magicBuffer = &mMagicBuffers[ ( int )commandDataCreateMagicBuffer->mMagicBufferHandle ];

      TAC_DX11_CALL( errors, mDevice->CreateBuffer, &desc, nullptr, &magicBuffer->mBuffer );
      SetDebugName( magicBuffer->mBuffer, commandDataCreateMagicBuffer->mStackFrame.ToString() );

      // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-structuredbuffer
      // The UAV format bound to this resource needs to be created with the DXGI_FORMAT_UNKNOWN format.
      const DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

      const int NumElements = desc.ByteWidth / desc.StructureByteStride;


      D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
      uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
      uavDesc.Format = Format;
      uavDesc.Buffer.FirstElement = 0; // index, not byte offset
      uavDesc.Buffer.Flags = 0;
      uavDesc.Buffer.NumElements = NumElements;
      TAC_DX11_CALL( errors, mDevice->CreateUnorderedAccessView, magicBuffer->mBuffer, &uavDesc, &magicBuffer->mUAV );
      SetDebugName( magicBuffer->mUAV, commandDataCreateMagicBuffer->mStackFrame.ToString() );

      D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Format = Format;
      srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; // D3D11_SRV_DIMENSION_BUFFEREX;
      srvDesc.Buffer.FirstElement = 0; // index, not byte offset
      srvDesc.Buffer.NumElements = NumElements;
      // srvDesc.BufferEx... = ;
      TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, magicBuffer->mBuffer, &srvDesc, &magicBuffer->mSRV );
      SetDebugName( magicBuffer->mSRV, commandDataCreateMagicBuffer->mStackFrame.ToString() );
    }

    void RendererDirectX11::AddVertexBuffer( Render::CommandDataCreateVertexBuffer* data,
                                             Errors& errors )
    {
      TAC_ASSERT( data->mStride );
      TAC_ASSERT( IsMainThread() );
      D3D11_BUFFER_DESC bd = {};
      bd.ByteWidth = data->mByteCount;
      bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      bd.Usage = GetUsage( data->mAccess );
      bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
      TAC_ASSERT( !data->mOptionalInitialBytes || Render::IsSubmitAllocated( data->mOptionalInitialBytes ) );
      D3D11_SUBRESOURCE_DATA initData = {};
      initData.pSysMem = data->mOptionalInitialBytes;
      D3D11_SUBRESOURCE_DATA* pInitData = data->mOptionalInitialBytes ? &initData : nullptr;
      ID3D11Buffer* buffer;
      TAC_DX11_CALL( errors,
                     mDevice->CreateBuffer,
                     &bd,
                     pInitData,
                     &buffer );
      VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )data->mVertexBufferHandle ];
      vertexBuffer->mBuffer = buffer;
      vertexBuffer->mStride = data->mStride;

      SetDebugName( buffer, data->mStackFrame.ToString() );
    }

    void RendererDirectX11::AddVertexFormat( Render::CommandDataCreateVertexFormat* commandData,
                                             Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      Render::VertexFormatHandle vertexFormatHandle = commandData->mVertexFormatHandle;
      Vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
      for( int iVertexFormatData = 0;
           iVertexFormatData < commandData->mVertexDeclarations.size();
           ++iVertexFormatData )
      {
        const VertexDeclaration& curFormat = commandData->mVertexDeclarations[ iVertexFormatData ];

        D3D11_INPUT_ELEMENT_DESC curDX11Input = {};
        curDX11Input.Format = GetDXGIFormat( curFormat.mTextureFormat );
        curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        curDX11Input.InstanceDataStepRate = 0;
        curDX11Input.InputSlot = 0;
        curDX11Input.SemanticName = GetSemanticName( curFormat.mAttribute );
        // MSDN:
        // A semantic index modifies a semantic, with an integer index number.
        // A semantic index is only needed in a case where there is more than
        // one element with the same semantic.
        curDX11Input.SemanticIndex;
        curDX11Input.AlignedByteOffset = curFormat.mAlignedByteOffset;
        inputElementDescs.push_back( curDX11Input );
      }
      ID3DBlob* inputSig = mPrograms[ ( int )commandData->mShaderHandle ].mInputSig;
      ID3D11InputLayout* inputLayout;
      TAC_DX11_CALL( errors,
                     mDevice->CreateInputLayout,
                     inputElementDescs.data(),
                     ( UINT )inputElementDescs.size(),
                     inputSig->GetBufferPointer(),
                     inputSig->GetBufferSize(),
                     &inputLayout );
      mInputLayouts[ ( int )vertexFormatHandle ] = inputLayout;
    }



    void RendererDirectX11::AddIndexBuffer( Render::CommandDataCreateIndexBuffer* data,
                                            Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      Render::IndexBufferHandle index = data->mIndexBufferHandle;
      TAC_ASSERT( data->mFormat.mPerElementDataType == GraphicsType::uint );
      TAC_ASSERT( data->mFormat.mElementCount == 1 );
      TAC_ASSERT( data->mFormat.mPerElementByteCount == 2 ||
                  data->mFormat.mPerElementByteCount == 4 );
      TAC_ASSERT( !data->mOptionalInitialBytes || Render::IsSubmitAllocated( data->mOptionalInitialBytes ) );
      D3D11_BUFFER_DESC bd = {};
      bd.ByteWidth = data->mByteCount;
      bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
      bd.Usage = GetUsage( data->mAccess );
      bd.CPUAccessFlags = data->mAccess == Access::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
      D3D11_SUBRESOURCE_DATA initData = {};
      initData.pSysMem = data->mOptionalInitialBytes;
      ID3D11Buffer* buffer;
      TAC_DX11_CALL( errors,
                     mDevice->CreateBuffer,
                     &bd,
                     data->mOptionalInitialBytes ? &initData : nullptr,
                     &buffer );
      mIndexBuffers[ ( int )index ].mFormat = data->mFormat;
      mIndexBuffers[ ( int )index ].mBuffer = buffer;
    }

    void RendererDirectX11::AddRasterizerState( Render::CommandDataCreateRasterizerState* commandData,
                                                Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      D3D11_RASTERIZER_DESC desc = {};
      desc.FillMode = GetFillMode( commandData->mRasterizerState.mFillMode );
      desc.CullMode = GetCullMode( commandData->mRasterizerState.mCullMode );
      desc.ScissorEnable = commandData->mRasterizerState.mScissor;
      desc.MultisampleEnable = commandData->mRasterizerState.mMultisample;
      desc.DepthClipEnable = true;
      desc.FrontCounterClockwise = commandData->mRasterizerState.mFrontCounterClockwise;
      ID3D11RasterizerState* rasterizerState;
      TAC_DX11_CALL( errors, mDevice->CreateRasterizerState, &desc, &rasterizerState );
      mRasterizerStates[ ( int )commandData->mRasterizerStateHandle ] = rasterizerState;
    }

    void RendererDirectX11::AddSamplerState( Render::CommandDataCreateSamplerState* commandData,
                                             Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      D3D11_SAMPLER_DESC desc = {};
      desc.Filter = GetFilter( commandData->mSamplerState.mFilter );
      desc.AddressU = GetAddressMode( commandData->mSamplerState.mU );
      desc.AddressV = GetAddressMode( commandData->mSamplerState.mV );
      desc.AddressW = GetAddressMode( commandData->mSamplerState.mW );
      desc.ComparisonFunc = GetCompare( commandData->mSamplerState.mCompare );
      desc.BorderColor[ 0 ] = 1;
      desc.BorderColor[ 1 ] = 0;
      desc.BorderColor[ 2 ] = 0;
      desc.BorderColor[ 3 ] = 1;
      ID3D11SamplerState* samplerStateDX11;
      TAC_DX11_CALL( errors, mDevice->CreateSamplerState, &desc, &samplerStateDX11 );
      mSamplerStates[ ( int )commandData->mSamplerStateHandle ] = samplerStateDX11;
    }



    void RendererDirectX11::AddShader( Render::CommandDataCreateShader* commandData,
                                       Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      mPrograms[ ( int )commandData->mShaderHandle ] = LoadProgram( commandData->mShaderSource, errors );
      if( commandData->mShaderSource.mType == Render::ShaderSource::Type::kPath )
        ShaderReloadHelperAdd( commandData->mShaderHandle, GetDirectX11ShaderPath( commandData->mShaderSource.mStr ) );
    }

    void RendererDirectX11::AddTexture( Render::CommandDataCreateTexture* data,
                                        Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      if( data->mTexSpec.mImageBytes && !data->mTexSpec.mPitch )
      {
        data->mTexSpec.mPitch =
          data->mTexSpec.mImage.mWidth *
          data->mTexSpec.mImage.mFormat.CalculateTotalByteCount();
      }

      // D3D11_SUBRESOURCE_DATA structure
      // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476220(v=vs.85).aspx
      // You set SysMemPitch to the distance between any two adjacent pixels on different lines.
      // You set SysMemSlicePitch to the size of the entire 2D surface in bytes.
      D3D11_SUBRESOURCE_DATA subResources[ 6 ] = {};
      int                    subResourceCount = 0;
      if( data->mTexSpec.mImageBytes )
      {
        D3D11_SUBRESOURCE_DATA* subResource = &subResources[ subResourceCount++ ];
        subResource->pSysMem = data->mTexSpec.mImageBytes;
        subResource->SysMemPitch = data->mTexSpec.mPitch;
        subResource->SysMemSlicePitch = data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight;
      }
      for( const void* imageBytesCubemap : data->mTexSpec.mImageBytesCubemap )
      {
        if( !imageBytesCubemap )
          continue;
        D3D11_SUBRESOURCE_DATA* subResource = &subResources[ subResourceCount++ ];
        subResource->pSysMem = imageBytesCubemap;
        subResource->SysMemPitch = data->mTexSpec.mPitch;
        subResource->SysMemSlicePitch = data->mTexSpec.mPitch * data->mTexSpec.mImage.mHeight;
      }
      const bool isCubemap = subResourceCount == 6;
      D3D11_SUBRESOURCE_DATA* pInitialData = subResourceCount ? subResources : nullptr;

      const UINT MiscFlagsBinding = GetMiscFlags( data->mTexSpec.mBinding );
      const UINT MiscFlagsCubemap = isCubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

      // remove this assert if we ever use texture arrays of arbitary size
      TAC_ASSERT( subResourceCount == 0 || // empty texture
                  subResourceCount == 1 || // single texture
                  subResourceCount == 6 ); // cubemap texture

      const int dimension
        = ( data->mTexSpec.mImage.mWidth ? 1 : 0 )
        + ( data->mTexSpec.mImage.mHeight ? 1 : 0 )
        + ( data->mTexSpec.mImage.mDepth ? 1 : 0 );

      const DXGI_FORMAT Format = GetDXGIFormat( data->mTexSpec.mImage.mFormat );

      ID3D11ShaderResourceView* srv = nullptr;
      ID3D11Texture2D* texture2D = nullptr;
      if( dimension == 2 )
      {
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = data->mTexSpec.mImage.mWidth;
        texDesc.Height = data->mTexSpec.mImage.mHeight;
        texDesc.MipLevels = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.ArraySize = Max( 1, subResourceCount );
        texDesc.Format = Format;
        texDesc.Usage = GetUsage( data->mTexSpec.mAccess );
        texDesc.BindFlags = GetBindFlags( data->mTexSpec.mBinding );
        texDesc.CPUAccessFlags = GetCPUAccessFlags( data->mTexSpec.mCpuAccess );
        texDesc.MiscFlags = MiscFlagsBinding | MiscFlagsCubemap;
        TAC_DX11_CALL( errors,
                       mDevice->CreateTexture2D,
                       &texDesc,
                       pInitialData,
                       &texture2D );
        SetDebugName( texture2D, data->mStackFrame.ToString() );
        if( ( int )data->mTexSpec.mBinding & ( int )Render::Binding::ShaderResource )
        {
          D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
          srvDesc.Format = Format;
          srvDesc.ViewDimension = isCubemap ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
          srvDesc.Texture2D.MipLevels = 1;
          TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, texture2D, &srvDesc, &srv );
          SetDebugName( srv, data->mStackFrame.ToString() );
        }
      }

      ID3D11Texture3D* texture3D = nullptr;
      if( dimension == 3 )
      {
        D3D11_TEXTURE3D_DESC texDesc = {};
        texDesc.Width = data->mTexSpec.mImage.mWidth;
        texDesc.Height = data->mTexSpec.mImage.mHeight;
        texDesc.Depth = data->mTexSpec.mImage.mDepth;
        texDesc.MipLevels = 1;
        texDesc.Format = Format;
        texDesc.Usage = GetUsage( data->mTexSpec.mAccess );
        texDesc.BindFlags = GetBindFlags( data->mTexSpec.mBinding );
        texDesc.CPUAccessFlags = GetCPUAccessFlags( data->mTexSpec.mCpuAccess );
        texDesc.MiscFlags = MiscFlagsBinding | MiscFlagsCubemap;
        TAC_DX11_CALL( errors,
                       mDevice->CreateTexture3D,
                       &texDesc,
                       pInitialData,
                       &texture3D );
        SetDebugName( texture3D, data->mStackFrame.ToString() );
        if( ( int )data->mTexSpec.mBinding & ( int )Render::Binding::ShaderResource )
        {
          D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
          srvDesc.Format = Format;
          srvDesc.ViewDimension = isCubemap ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE3D;
          srvDesc.Texture3D.MipLevels = 1;
          TAC_DX11_CALL( errors, mDevice->CreateShaderResourceView, texture3D, &srvDesc, &srv );
        }
      }

      ID3D11RenderTargetView* rTV = nullptr;
      if( ( int )data->mTexSpec.mBinding & ( int )Render::Binding::RenderTarget )
      {
        TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
                       texture2D,
                       nullptr,
                       &rTV );
        SetDebugName( rTV, data->mStackFrame.ToString() );
      }



      //ID3D11UnorderedAccessView* uav = nullptr;
      //if( ( int )data->mTexSpec.mBinding & ( int )Render::Binding::UnorderedAccess )
      //{
      //}

      if( true
          && ( int )data->mTexSpec.mBinding & ( int )Render::Binding::ShaderResource
          && ( int )data->mTexSpec.mBinding & ( int )Render::Binding::RenderTarget )
        mDeviceContext->GenerateMips( srv );

      Texture* texture = &mTextures[ ( int )data->mTextureHandle ];
      texture->mTexture2D = texture2D;
      texture->mTexture3D = texture3D;
      texture->mTextureSRV = srv;
      texture->mTextureRTV = rTV;
    }

    void RendererDirectX11::AddBlendState( Render::CommandDataCreateBlendState* commandData,
                                           Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      Render::BlendStateHandle blendStateHandle = commandData->mBlendStateHandle;
      Render::BlendState* blendState = &commandData->mBlendState;
      D3D11_BLEND_DESC desc = {};
      D3D11_RENDER_TARGET_BLEND_DESC* d3d11rtbd = &desc.RenderTarget[ 0 ];
      d3d11rtbd->BlendEnable = true;
      d3d11rtbd->SrcBlend = GetBlend( blendState->mSrcRGB );
      d3d11rtbd->DestBlend = GetBlend( blendState->mDstRGB );
      d3d11rtbd->BlendOp = GetBlendOp( blendState->mBlendRGB );
      d3d11rtbd->SrcBlendAlpha = GetBlend( blendState->mSrcA );
      d3d11rtbd->DestBlendAlpha = GetBlend( blendState->mDstA );
      d3d11rtbd->BlendOpAlpha = GetBlendOp( blendState->mBlendA );
      d3d11rtbd->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      ID3D11BlendState* blendStateDX11;
      TAC_DX11_CALL( errors, mDevice->CreateBlendState, &desc, &blendStateDX11 );
      mBlendStates[ ( int )blendStateHandle ] = blendStateDX11;
    }

    void RendererDirectX11::AddConstantBuffer( Render::CommandDataCreateConstantBuffer* commandData,
                                               Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      ID3D11Buffer* cbufferhandle;
      D3D11_BUFFER_DESC bd = {};
      bd.ByteWidth = RoundUpToNearestMultiple( commandData->mByteCount, 16 );
      bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // i guess?
      bd.Usage = D3D11_USAGE_DYNAMIC; // i guess?
      TAC_DX11_CALL( errors, mDevice->CreateBuffer, &bd, nullptr, &cbufferhandle );

      ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )commandData->mConstantBufferHandle ];
      constantBuffer->mBuffer = cbufferhandle;
      constantBuffer->mShaderRegister = commandData->mShaderRegister;
    }

    void RendererDirectX11::AddDepthState( Render::CommandDataCreateDepthState* commandData,
                                           Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      D3D11_DEPTH_STENCIL_DESC desc = {};
      desc.DepthFunc = GetDepthFunc( commandData->mDepthState.mDepthFunc );
      desc.DepthEnable = commandData->mDepthState.mDepthTest;
      desc.DepthWriteMask
        = commandData->mDepthState.mDepthWrite
        ? D3D11_DEPTH_WRITE_MASK_ALL
        : D3D11_DEPTH_WRITE_MASK_ZERO;
      ID3D11DepthStencilState* depthStencilState;
      TAC_DX11_CALL( errors, mDevice->CreateDepthStencilState, &desc, &depthStencilState );
      mDepthStencilStates[ ( int )commandData->mDepthStateHandle ] = depthStencilState;
    }

    void RendererDirectX11::AddFramebuffer( Render::CommandDataCreateFramebuffer* data,
                                            Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );

      Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
      const bool isWindowFramebuffer = data->mNativeWindowHandle && data->mWidth && data->mHeight;
      const bool isRenderToTextureFramebuffer = !data->mFramebufferTextures.empty();
      TAC_ASSERT( isWindowFramebuffer || isRenderToTextureFramebuffer );

      if( isWindowFramebuffer )
      {
        TAC_ASSERT( data->mWidth );

        auto hwnd = ( HWND )data->mNativeWindowHandle;
        IDXGISwapChain* swapChain;
        const int bufferCount = 4;
        const UINT width = data->mWidth;
        const UINT height = data->mHeight;
        DXGICreateSwapChain( hwnd,
                             mDevice,
                             bufferCount,
                             width,
                             height,
                             &swapChain,
                             errors );
        TAC_HANDLE_ERROR( errors );

        ID3D11Device* device = mDevice;
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChain->GetDesc( &swapChainDesc );

        ID3D11Texture2D* pBackBuffer = nullptr;
        TAC_DXGI_CALL( errors, swapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
        TAC_HANDLE_ERROR_IF( !pBackBuffer, "no buffer to resize", errors );
        ID3D11RenderTargetView* rtv = nullptr;
        D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
        TAC_DX11_CALL( errors, device->CreateRenderTargetView, pBackBuffer, rtvDesc, &rtv );
        pBackBuffer->Release();

        D3D11_RENDER_TARGET_VIEW_DESC createdDesc = {};
        rtv->GetDesc( &createdDesc );

        TAC_ASSERT( IsMainThread() );
        D3D11_TEXTURE2D_DESC texture2dDesc = {};
        texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        texture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        texture2dDesc.Height = height;
        texture2dDesc.Width = width;
        texture2dDesc.SampleDesc.Count = 1;
        texture2dDesc.SampleDesc.Quality = 0;
        texture2dDesc.ArraySize = 1;
        texture2dDesc.MipLevels = 1;

        ID3D11Texture2D* texture;
        TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &texture2dDesc, nullptr, &texture );

        ID3D11DepthStencilView* dsv;
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
        depthStencilViewDesc.Format = texture2dDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, texture, &depthStencilViewDesc, &dsv );

        framebuffer->mSwapChain = swapChain;
        framebuffer->mDepthStencilView = dsv;
        framebuffer->mDepthTexture = texture;
        framebuffer->mHwnd = hwnd;
        framebuffer->mRenderTargetView = rtv;
        framebuffer->mBufferCount = bufferCount;

        mWindows[ mWindowCount++ ] = data->mFramebufferHandle;
      }
      else if( isRenderToTextureFramebuffer )
      {
        for( TextureHandle textureHandle : data->mFramebufferTextures )
        {
          TAC_ASSERT( textureHandle.IsValid() );
          Texture* texture = &mTextures[ ( int )textureHandle ];

          D3D11_TEXTURE2D_DESC desc;
          texture->mTexture2D->GetDesc( &desc );

          const bool isDepthTexture =
            desc.Format == DXGI_FORMAT_D16_UNORM ||
            desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
            desc.Format == DXGI_FORMAT_D32_FLOAT ||
            desc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
          if( isDepthTexture )
          {
            D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
            depthStencilViewDesc.Format = desc.Format;
            depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            TAC_DX11_CALL( errors,
                           mDevice->CreateDepthStencilView,
                           texture->mTexture2D,
                           &depthStencilViewDesc,
                           &framebuffer->mDepthStencilView );
            framebuffer->mDepthTexture = texture->mTexture2D;
          }
          else
          {
            framebuffer->mRenderTargetView = texture->mTextureRTV;
          }
        }
      }
      else
      {
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
      }
    }

    void RendererDirectX11::RemoveVertexBuffer( Render::VertexBufferHandle vertexBufferHandle, Errors& errors )
    {
      VertexBuffer* vertexBuffer = &mVertexBuffers[ ( int )vertexBufferHandle ];
      TAC_RELEASE_IUNKNOWN( vertexBuffer->mBuffer );
      *vertexBuffer = VertexBuffer();
    }

    void RendererDirectX11::RemoveVertexFormat( Render::VertexFormatHandle vertexFormatHandle, Errors& )
    {
      TAC_RELEASE_IUNKNOWN( mInputLayouts[ ( int )vertexFormatHandle ] );
    }

    void RendererDirectX11::RemoveIndexBuffer( Render::IndexBufferHandle indexBufferHandle, Errors& )
    {
      IndexBuffer* indexBuffer = &mIndexBuffers[ ( int )indexBufferHandle ];
      TAC_RELEASE_IUNKNOWN( indexBuffer->mBuffer );
      *indexBuffer = IndexBuffer();
    }

    void RendererDirectX11::RemoveRasterizerState( Render::RasterizerStateHandle rasterizerStateHandle, Errors& )
    {
      TAC_RELEASE_IUNKNOWN( mRasterizerStates[ ( int )rasterizerStateHandle ] );
    }

    void RendererDirectX11::RemoveSamplerState( Render::SamplerStateHandle samplerStateHandle, Errors& )
    {
      TAC_RELEASE_IUNKNOWN( mSamplerStates[ ( int )samplerStateHandle ] );
    }

    void RendererDirectX11::RemoveShader( const Render::ShaderHandle shaderHandle, Errors& )
    {
      Program* program = &mPrograms[ ( int )shaderHandle ];
      TAC_RELEASE_IUNKNOWN( program->mInputSig );
      TAC_RELEASE_IUNKNOWN( program->mVertexShader );
      TAC_RELEASE_IUNKNOWN( program->mPixelShader );
      TAC_RELEASE_IUNKNOWN( program->mGeometryShader );
      ShaderReloadHelperRemove( shaderHandle );
    }

    void RendererDirectX11::RemoveTexture( Render::TextureHandle textureHandle, Errors& )
    {
      Texture* texture = &mTextures[ ( int )textureHandle ];
      TAC_RELEASE_IUNKNOWN( texture->mTexture2D );
      TAC_RELEASE_IUNKNOWN( texture->mTextureRTV );
      TAC_RELEASE_IUNKNOWN( texture->mTextureSRV );
    }

    void RendererDirectX11::RemoveFramebuffer( Render::FramebufferHandle framebufferHandle, Errors& )
    {
      for( int i = 0; i < mWindowCount; ++i )
        if( mWindows[ i ] == framebufferHandle )
          mWindows[ i ] = mWindows[ --mWindowCount ];
      Framebuffer* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];
      TAC_RELEASE_IUNKNOWN( framebuffer->mDepthStencilView );
      TAC_RELEASE_IUNKNOWN( framebuffer->mDepthTexture );
      TAC_RELEASE_IUNKNOWN( framebuffer->mRenderTargetView );
      TAC_RELEASE_IUNKNOWN( framebuffer->mSwapChain );
      *framebuffer = Framebuffer();
    }

    void RendererDirectX11::RemoveBlendState( Render::BlendStateHandle blendStateHandle, Errors& )
    {
      TAC_RELEASE_IUNKNOWN( mBlendStates[ ( int )blendStateHandle ] );
    }

    void RendererDirectX11::RemoveConstantBuffer( Render::ConstantBufferHandle constantBufferHandle, Errors& )
    {
      ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )constantBufferHandle ];
      TAC_RELEASE_IUNKNOWN( constantBuffer->mBuffer );
      *constantBuffer = ConstantBuffer();
    }

    void RendererDirectX11::RemoveDepthState( Render::DepthStateHandle depthStateHandle, Errors& )
    {
      TAC_RELEASE_IUNKNOWN( mDepthStencilStates[ ( int )depthStateHandle ] );
    }

    void RendererDirectX11::UpdateTextureRegion( Render::CommandDataUpdateTextureRegion* commandData,
                                                 Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      Render::TexUpdate* data = &commandData->mTexUpdate;
      TAC_ASSERT( Render::IsSubmitAllocated( data->mSrcBytes ) );

      const UINT dstX = data->mDstX;
      const UINT dstY = data->mDstY;
      const UINT dstZ = 0;

      D3D11_BOX srcBox = {};
      srcBox.right = data->mSrc.mWidth;
      srcBox.bottom = data->mSrc.mHeight;
      srcBox.back = 1;

      D3D11_TEXTURE2D_DESC texDesc = {};
      texDesc.Width = data->mSrc.mWidth;
      texDesc.Height = data->mSrc.mHeight;
      texDesc.MipLevels = 1;
      texDesc.SampleDesc.Count = 1;
      texDesc.ArraySize = 1;
      texDesc.Format = GetDXGIFormat( data->mSrc.mFormat );
      texDesc.Usage = D3D11_USAGE_DEFAULT;
      texDesc.BindFlags = 0;
      texDesc.CPUAccessFlags = 0;
      texDesc.MiscFlags = 0;

      D3D11_SUBRESOURCE_DATA subResource = {};
      subResource.pSysMem = data->mSrcBytes;
      subResource.SysMemPitch = data->mPitch;
      subResource.SysMemSlicePitch = data->mPitch * data->mSrc.mHeight;

      ID3D11Resource* dstTex = mTextures[ ( int )commandData->mTextureHandle ].mTexture2D;
      ID3D11Texture2D* srcTex;
      TAC_DX11_CALL( errors,
                     mDevice->CreateTexture2D,
                     &texDesc,
                     &subResource,
                     &srcTex );

      mDeviceContext->CopySubresourceRegion( dstTex,
                                             0, // dst subresource
                                             dstX,
                                             dstY,
                                             dstZ,
                                             srcTex,
                                             0, // src subresource,
                                             &srcBox );
      TAC_RELEASE_IUNKNOWN( srcTex );
    }

    void RendererDirectX11::UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer* commandData,
                                                Errors& errors )
    {
      ID3D11Buffer* buffer = mVertexBuffers[ ( int )commandData->mVertexBufferHandle ].mBuffer;
      UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
    }

    void RendererDirectX11::UpdateConstantBuffer( Render::CommandDataUpdateConstantBuffer* commandData,
                                                  Errors& errors )
    {
      const ConstantBuffer* constantBuffer = &mConstantBuffers[ ( int )commandData->mConstantBufferHandle ];
      UpdateBuffer( constantBuffer->mBuffer,
                    commandData->mBytes,
                    commandData->mByteCount,
                    errors );

      mBoundConstantBuffers[ constantBuffer->mShaderRegister ] = constantBuffer->mBuffer;
      mBoundConstantBufferCount = Max( mBoundConstantBufferCount, constantBuffer->mShaderRegister + 1 );

      const UINT StartSlot = 0;
      mDeviceContext->PSSetConstantBuffers( StartSlot, mBoundConstantBufferCount, mBoundConstantBuffers );
      mDeviceContext->VSSetConstantBuffers( StartSlot, mBoundConstantBufferCount, mBoundConstantBuffers );
    }

    void RendererDirectX11::UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer* commandData,
                                               Errors& errors )
    {
      ID3D11Buffer* buffer = mIndexBuffers[ ( int )commandData->mIndexBufferHandle ].mBuffer;
      UpdateBuffer( buffer, commandData->mBytes, commandData->mByteCount, errors );
    }

    void RendererDirectX11::ResizeFramebuffer( Render::CommandDataResizeFramebuffer* data,
                                               Errors& errors )
    {
      Render::FramebufferHandle framebufferHandle = data->mFramebufferHandle;

      Framebuffer* framebuffer = &mFramebuffers[ ( int )data->mFramebufferHandle ];
      IDXGISwapChain* swapChain = framebuffer->mSwapChain;

      D3D11_TEXTURE2D_DESC depthTextureDesc;
      framebuffer->mDepthTexture->GetDesc( &depthTextureDesc );
      depthTextureDesc.Width = data->mWidth;
      depthTextureDesc.Height = data->mHeight;

      // Release outstanding back buffer references prior to calling IDXGISwapChain::ResizeBuffers
      framebuffer->mDepthStencilView->Release();
      framebuffer->mDepthStencilView = nullptr;
      framebuffer->mDepthTexture->Release();
      framebuffer->mDepthTexture = nullptr;
      framebuffer->mRenderTargetView->Release();
      framebuffer->mRenderTargetView = nullptr;

      DXGI_SWAP_CHAIN_DESC desc;
      TAC_HANDLE_ERROR_IF( FAILED( framebuffer->mSwapChain->GetDesc( &desc ) ),
                           "Failed to get swap chain desc",
                           errors );
      framebuffer->mSwapChain->ResizeBuffers( framebuffer->mBufferCount,
                                              data->mWidth,
                                              data->mHeight,
                                              DXGI_FORMAT_UNKNOWN,
                                              desc.Flags );
      ID3D11Texture2D* pBackBuffer = nullptr;
      TAC_DXGI_CALL( errors, swapChain->GetBuffer, 0, IID_PPV_ARGS( &pBackBuffer ) );
      TAC_HANDLE_ERROR_IF( !pBackBuffer, "no buffer to resize", errors );
      ID3D11RenderTargetView* rtv = nullptr;
      D3D11_RENDER_TARGET_VIEW_DESC* rtvDesc = nullptr;
      TAC_DX11_CALL( errors, mDevice->CreateRenderTargetView,
                     pBackBuffer,
                     rtvDesc,
                     &rtv );
      pBackBuffer->Release();

      ID3D11Texture2D* depthTexture;
      TAC_DX11_CALL( errors, mDevice->CreateTexture2D, &depthTextureDesc, nullptr, &depthTexture );

      ID3D11DepthStencilView* dsv;
      D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
      depthStencilViewDesc.Format = depthTextureDesc.Format;
      depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      TAC_DX11_CALL( errors, mDevice->CreateDepthStencilView, depthTexture, &depthStencilViewDesc, &dsv );

      framebuffer->mDepthStencilView = dsv;
      framebuffer->mDepthTexture = depthTexture;
      framebuffer->mRenderTargetView = rtv;
    }

    void RendererDirectX11::SetRenderObjectDebugName( Render::CommandDataSetRenderObjectDebugName* data,
                                                      Errors& errors )
    {
      FrameMemoryVector< ID3D11DeviceChild* > objects;
      if( data->mVertexBufferHandle.IsValid() )
        objects.push_back( mVertexBuffers[ ( int )data->mVertexBufferHandle ].mBuffer );
      else if( data->mIndexBufferHandle.IsValid() )
        objects.push_back( mIndexBuffers[ ( int )data->mIndexBufferHandle ].mBuffer );
      else if( data->mTextureHandle.IsValid() )
        objects.push_back( mTextures[ ( int )data->mTextureHandle ].mTexture2D );
      else
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
      for( ID3D11DeviceChild* object : objects )
        SetDebugName( object, data->mName );
    }

    void RendererDirectX11::UpdateBuffer( ID3D11Buffer* buffer,
                                          const void* bytes,
                                          int byteCount,
                                          Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      TAC_ASSERT( Render::IsSubmitAllocated( bytes ) );
      D3D11_MAPPED_SUBRESOURCE mappedResource;
      TAC_DX11_CALL( errors, mDeviceContext->Map, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
      MemCpy( mappedResource.pData, bytes, byteCount );
      mDeviceContext->Unmap( buffer, 0 );
    }


    void RendererDirectX11::DebugGroupBegin( StringView desc )
    {
      if( !mUserAnnotationDEBUG )
        return;
      TAC_ASSERT( !desc.empty() );
      LPCWSTR descWchar = ToTransientWchar( desc );
      mUserAnnotationDEBUG->BeginEvent( descWchar );

    }
    void RendererDirectX11::DebugMarker( StringView desc )
    {
      if( !mUserAnnotationDEBUG )
        return;
      LPCWSTR descWchar = ToTransientWchar( desc );
      mUserAnnotationDEBUG->SetMarker( descWchar );

    }
    void RendererDirectX11::DebugGroupEnd()
    {

      if( mUserAnnotationDEBUG )
        mUserAnnotationDEBUG->EndEvent();

    }


  } // namespace Render
} // namespace Tac
