#pragma once

#define MICROTEX_VERSION_MAJOR 0
#define MICROTEX_VERSION_MINOR 0
#define MICROTEX_VERSION_PATCH 0

#if defined( _MSC_VER )
#pragma warning( disable: 4099 ) // type name first seen using 'class' now seen using 'struct'
#pragma warning( disable: 4100 ) // unreferenced parameter
#pragma warning( disable: 4101 ) // unreferenced local var
#pragma warning( disable: 4244 ) // conversion possible loss of data
#pragma warning( disable: 4251 ) // needs to have dll-interface
#pragma warning( disable: 4267 ) // conversion possible loss of data
#pragma warning( disable: 4456 ) // declaration hides previous local declaration
#pragma warning( disable: 4457 ) // declaration hides function parameter
#pragma warning( disable: 4996 ) // 'fopen' unsafe
#endif
