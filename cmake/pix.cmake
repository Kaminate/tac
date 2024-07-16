# =============================================================================
# Helper function to introduce a target representing a PIX event runtime.
# =============================================================================

# based off:
# https://github.com/microsoft/DirectML/blob/master/DxDispatch/cmake/pix.cmake

# see also:
# https://devblogs.microsoft.com/pix/winpixeventruntime/

# usage: (to link PIX to your `my_cool_target` )
#
#   include( cmake/pix.cmake )
#   add_pix_target()
#   target_link_libraries( my_cool_target PIX )

include_guard()
include( FetchContent )

# -----------------------------------------------------------------------------
# Init using a NuGet distribution.
# -----------------------------------------------------------------------------
function( init_pix_target_nuget
          pkg_id
          pkg_version
          pkg_hash )


endfunction()


# -----------------------------------------------------------------------------
# Main function to add the target.
# -----------------------------------------------------------------------------
function( add_pix_target )

    add_library( PIX INTERFACE )

    
    FetchContent_Declare(
      pix_content
      URL "https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.230302001"
      URL_HASH SHA256=1CC9C6618A00F26375A8D98ADBA60620904FBF6A8E71007E14439CA01436589D )

    # FetchContent_Declare(
    #   pix_content
    #   URL "https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001"
    #   URL_HASH SHA256=726ACC93D6968E2146261A1E415521747D50AD69894C2B42B5D0D4C29FD66EC4 )

    FetchContent_MakeAvailable( pix_content )

    message( "pix content src dir: " ${pix_content_SOURCE_DIR})

    target_include_directories( PIX INTERFACE "${pix_content_SOURCE_DIR}/include")
    target_link_libraries( PIX INTERFACE "${pix_content_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib")
    target_compile_definitions( PIX INTERFACE USE_PIX )
    set_property( TARGET PIX PROPERTY DX_COMPONENT_CONFIG "NuGet (WinPixEventRuntime.1.0.230302001)" )
    # set_property( TARGET PIX PROPERTY DX_COMPONENT_CONFIG "NuGet (WinPixEventRuntime.1.0.240308001)" )

    # install the dll?

    message( "cmake install prefi: " ${CMAKE_INSTALL_PREFIX} )
    message( "cmake install bin: " ${CMAKE_INSTALL_BINDIR} )

    set( PIX_DLL_PATH ${pix_content_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll
         CACHE FILEPATH "Path of PIX dll" )



endfunction()
