#if _MSC_VER
#pragma warning( disable : 4996 ) // fopen unsafe, use fopen_s
#pragma warning( disable : 6385 ) // reading invalid data from header (?!)
#pragma warning( disable : 6387 ) // buffer could be 0
#endif

#define CGLTF_WRITE_IMPLEMENTATION
#include "tac-engine-core/thirdparty/cgltf/cgltf_write.h"

