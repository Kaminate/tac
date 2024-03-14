#pragma once


namespace Tac
{
  struct String;

  struct FormatByteSpec
  {
    int mByteCount = -1;

    // Set to 1 for bytes,
    // Set to 1024 for kb
    // Set to 1024 * 1024 for mb, etc
    int mMinDenomination = 1;
  };

  String FormatBytes( const FormatByteSpec& );
  String FormatBytes( int );
}

