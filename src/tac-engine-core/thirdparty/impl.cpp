#include "tac-std-lib/preprocess/tac_preprocessor.h" // include tac warning disables

// additional warning disables
#if _MSC_VER
#pragma warning( disable : 26451 )
#pragma warning( disable : 26819 )
#pragma warning( disable : 28182 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 6001 )
#pragma warning( disable : 6262 )
#pragma warning( disable : 6308 )
#endif

#define CGLTF_IMPLEMENTATION
#include "tac-std-lib/thirdparty/cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

