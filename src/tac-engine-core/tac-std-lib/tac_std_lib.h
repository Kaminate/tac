#pragma once

// this file is a bunch of forward declares so that you can include 1 file
// and not have to forward declare lots of things
namespace Tac
{
  struct Camera;

  // math
  struct v2;
  struct v2i;
  struct v3;
  struct v3i;
  struct v4;
  struct m2;
  struct m3;
  struct m4;


  // string
  struct String;
  struct StringView;
  struct StringID;
  struct StringLiteral;
  struct ShortFixedString;

  struct Errors;
  struct Json;
  struct StackFrame;
  struct Mesh;


  // Serialization
  struct Reader;
  struct Writer;

  struct DesktopWindowHandle;
  struct DesktopWindowRect;

  struct Timestamp;
  struct TimestampDifference;
}

namespace Tac::Filesystem
{
  struct Path;
}

//namespace Tac::Render
//{
//  struct TextureHandle;
//}


