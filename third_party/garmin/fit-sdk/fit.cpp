/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2025 Garmin International, Inc.
// Licensed under the Flexible and Interoperable Data Transfer (FIT) Protocol License; you
// may not use this file except in compliance with the Flexible and Interoperable Data
// Transfer (FIT) Protocol License.
/////////////////////////////////////////////////////////////////////////////////////////////
// ****WARNING****  This file is auto-generated!  Do NOT edit this file.
// Profile Version = 21.171.0Release
// Tag = production/release/21.171.0-0-g57fed75
/////////////////////////////////////////////////////////////////////////////////////////////


#include "fit.hpp"

///////////////////////////////////////////////////////////////////////
// Private Definitions
///////////////////////////////////////////////////////////////////////

namespace fit
{

///////////////////////////////////////////////////////////////////////
// Public Constants
///////////////////////////////////////////////////////////////////////

const FIT_UINT8 baseTypeSizes[FIT_BASE_TYPES] =
    {
        sizeof(FIT_ENUM),
        sizeof(FIT_SINT8),
        sizeof(FIT_UINT8),
        sizeof(FIT_SINT16),
        sizeof(FIT_UINT16),
        sizeof(FIT_SINT32),
        sizeof(FIT_UINT32),
        sizeof(FIT_STRING),
        sizeof(FIT_FLOAT32),
        sizeof(FIT_FLOAT64),
        sizeof(FIT_UINT8Z),
        sizeof(FIT_UINT16Z),
        sizeof(FIT_UINT32Z),
        sizeof(FIT_BYTE),
        sizeof(FIT_SINT64),
        sizeof(FIT_UINT64),
        sizeof(FIT_UINT64Z),
    };

union FIT_FLOAT
    {
    FIT_UINT8       uint8_value[8];
    FIT_FLOAT64     float64_value;
    FIT_FLOAT32     float32_value;
    };

static const FIT_FLOAT floatInvalid =
    {
        {
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF
        }
    };

const FIT_ENUM enumInvalid = FIT_ENUM_INVALID;
const FIT_SINT8 sint8Invalid = FIT_SINT8_INVALID;
const FIT_UINT8 uint8Invalid = FIT_UINT8_INVALID;
const FIT_SINT16 sint16Invalid = FIT_SINT16_INVALID;
const FIT_UINT16 uint16Invalid = FIT_UINT16_INVALID;
const FIT_SINT32 sint32Invalid = FIT_SINT32_INVALID;
const FIT_UINT32 uint32Invalid = FIT_UINT32_INVALID;
const FIT_STRING stringInvalid = FIT_STRING_INVALID;
const FIT_UINT8Z uint8zInvalid = FIT_UINT8Z_INVALID;
const FIT_UINT16Z uint16zInvalid = FIT_UINT16Z_INVALID;
const FIT_UINT32Z uint32zInvalid = FIT_UINT32Z_INVALID;
const FIT_BYTE byteInvalid = FIT_BYTE_INVALID;
const FIT_SINT64 sint64Invalid = FIT_SINT64_INVALID;
const FIT_UINT64 uint64Invalid = FIT_UINT64_INVALID;
const FIT_UINT64Z uint64zInvalid = FIT_UINT64Z_INVALID;

const FIT_UINT8 *baseTypeInvalids[FIT_BASE_TYPES] =
    {
        (FIT_UINT8 *)&enumInvalid,
        (FIT_UINT8 *)&sint8Invalid,
        (FIT_UINT8 *)&uint8Invalid,
        (FIT_UINT8 *)&sint16Invalid,
        (FIT_UINT8 *)&uint16Invalid,
        (FIT_UINT8 *)&sint32Invalid,
        (FIT_UINT8 *)&uint32Invalid,
        (FIT_UINT8 *)&stringInvalid,
        (FIT_UINT8 *)&floatInvalid.float32_value,
        (FIT_UINT8 *)&floatInvalid.float64_value,
        (FIT_UINT8 *)&uint8zInvalid,
        (FIT_UINT8 *)&uint16zInvalid,
        (FIT_UINT8 *)&uint32zInvalid,
        (FIT_UINT8 *)&byteInvalid,
        (FIT_UINT8 *)&sint64Invalid,
        (FIT_UINT8 *)&uint64Invalid,
        (FIT_UINT8 *)&uint64zInvalid,
    };

const std::map<ProtocolVersion, DetailedProtocolVersion> versionMap
{
    { ProtocolVersion::V10, DetailedProtocolVersion( 1, 0 ) },
    { ProtocolVersion::V20, DetailedProtocolVersion( 2, 0 ) }
};

///////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////

DetailedProtocolVersion::DetailedProtocolVersion(FIT_UINT8 major, FIT_UINT8 minor)
{
    version = ( major << FIT_PROTOCOL_VERSION_MAJOR_SHIFT ) | minor;
}

FIT_UINT8 DetailedProtocolVersion::GetMajorVersion() const
{
    return ( version & FIT_PROTOCOL_VERSION_MAJOR_MASK ) >> FIT_PROTOCOL_VERSION_MAJOR_SHIFT;
}

FIT_UINT8 DetailedProtocolVersion::GetMinorVersion() const
{
    return version & FIT_PROTOCOL_VERSION_MINOR_MASK;
}

FIT_UINT8 DetailedProtocolVersion::GetVersionByte() const
{
    return version;
}

FIT_UINT8 GetArch(void)
{
   const FIT_UINT16 arch = 0x0100;
   return (*(FIT_UINT8 *)&arch);
}

} // namespace fit

#if defined(FIT_CPP_INCLUDE_C)
    // Include C Implementation
    #include "fit.c"
#endif
