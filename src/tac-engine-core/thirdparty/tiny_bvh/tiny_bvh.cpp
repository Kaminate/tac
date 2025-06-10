#include "tac-std-lib/preprocess/tac_preprocessor.h" // include tac warning disables

// additional warning disables
#if _MSC_VER
//#pragma warning( disable : 26451 )
//#pragma warning( disable : 26819 )
//#pragma warning( disable : 28182 )
//#pragma warning( disable : 4100 )
//#pragma warning( disable : 4244 )
//#pragma warning( disable : 4456 )
//#pragma warning( disable : 4996 )
//#pragma warning( disable : 6001 )
//#pragma warning( disable : 6262 )
//#pragma warning( disable : 6308 )
#endif

#define TINYBVH_IMPLEMENTATION
#define NO_DOUBLE_PRECISION_SUPPORT
#include "tiny_bvh.h"

