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


#include <cmath>
#include <sstream>
#include "fit_field_base.hpp"
#include "fit_mesg.hpp"
#include "fit_unicode.hpp"

namespace fit
{

FieldBase::FieldBase(void)
{
}

FieldBase::FieldBase(const FieldBase &field)
{
    values = field.values;
    stringIndexes = field.stringIndexes;
}

FieldBase::~FieldBase()
{
}

std::string FieldBase::GetName(const FIT_UINT16 subFieldIndex) const
{
    if (subFieldIndex >= GetNumSubFields())
        return GetName();

    auto subfield = GetSubField(subFieldIndex);
    return NULL != subfield ? subfield->name : "unknown";
}

FIT_UINT8 FieldBase::GetType(const FIT_UINT16 subFieldIndex) const
{
    if (subFieldIndex >= GetNumSubFields())
        return GetType();

    auto subfield = GetSubField(subFieldIndex);
    return NULL != subfield ? subfield->type : FIT_UINT8_INVALID;
}

std::string FieldBase::GetUnits(const FIT_UINT16 subFieldIndex) const
{
    if (subFieldIndex >= GetNumSubFields())
        return GetUnits();

    auto subfield = GetSubField(subFieldIndex);
    return NULL != subfield ? subfield->units : "";
}

FIT_FLOAT64 FieldBase::GetScale(const FIT_UINT16 subFieldIndex) const
{
    if (subFieldIndex >= GetNumSubFields())
        return GetScale();

    auto subfield = GetSubField(subFieldIndex);
    return NULL != subfield ? subfield->scale : 1.0;
}

FIT_FLOAT64 FieldBase::GetOffset(const FIT_UINT16 subFieldIndex) const
{
    if (subFieldIndex >= GetNumSubFields())
        return GetOffset();

    auto subfield = GetSubField(subFieldIndex);
    return NULL != subfield ? subfield->offset : 0.0;
}

FIT_BOOL FieldBase::IsSignedInteger(const FIT_UINT16 subFieldIndex) const
{
    switch (GetType(subFieldIndex)) {
        case FIT_BASE_TYPE_SINT8:
        case FIT_BASE_TYPE_SINT16:
        case FIT_BASE_TYPE_SINT32:
            return FIT_TRUE;

        default:
            return FIT_FALSE;
    }
}

FIT_UINT8 FieldBase::GetSize(void) const
{
    if (!IsValid())
        return 0;

    return (FIT_UINT8)values.size();
}

FIT_UINT8 FieldBase::GetNumValues(void) const
{
    if (!IsValid())
        return 0;

    if (GetType() != FIT_BASE_TYPE_STRING)
        return (FIT_UINT8)(values.size() / baseTypeSizes[GetType() & FIT_BASE_TYPE_NUM_MASK]);

    return (FIT_UINT8)stringIndexes.size();
}

FIT_UINT32 FieldBase::GetBitsValue(const FIT_UINT16 offset, const FIT_UINT8 bits) const
{
    FIT_UINT32 value = 0;
    FIT_UINT8 numValueBits = 0;
    FIT_UINT8 numDataBitsUsed;
    FIT_UINT8 index = 0;
    FIT_UINT8 data;
    FIT_UINT8 mask;
    FIT_UINT16 newOffset = offset;

    if (values.size() == 0)
        return FIT_UINT32_INVALID;

    while (numValueBits < bits)
    {
        if (index >= GetSize())
            return FIT_UINT32_INVALID;

        if (newOffset >= 8)
        {
            newOffset -= 8;
        }
        else
        {
            // Get Nth byte from the jagged array considering elements may be multibyte
            data = GetValuesUINT8(index);
            // Shift out the bits we do not want to use
            data >>= newOffset;
            // Record how many bits we are using
            numDataBitsUsed = 8 - (FIT_UINT8)newOffset;
            // Use every byte after this until we have enough bits
            newOffset = 0;

            // If the number of data bits will overflow the number of bits we want in the
            // final value, only use as many as will fit
            if (numDataBitsUsed > (bits - numValueBits))
                numDataBitsUsed = bits - numValueBits;

            mask = (1 << numDataBitsUsed) - 1;
            // OR the data from this byte into our final value, then update number of bits
            // in our final value
            value |= (FIT_UINT32)(data & mask) << numValueBits;
            numValueBits += numDataBitsUsed;
        }
        index++;
    }

    return value;
}

FIT_SINT32 FieldBase::GetBitsSignedValue(const FIT_UINT16 offset, const FIT_UINT8 bits) const
{
    FIT_UINT32 value;
    FIT_SINT32 signedValue;

    value = GetBitsValue(offset, bits);

    if (value == FIT_UINT32_INVALID)
        return FIT_SINT32_INVALID;

    signedValue = (1 << (bits - 1));

    if ((value & signedValue) != 0) // sign bit set
        signedValue = -signedValue + (value & (signedValue - 1));
    else
        signedValue = value;

    return signedValue;
}

FIT_BYTE FieldBase::GetValuesBYTE(FIT_UINT8 index) const
{
    /// Returns the Nth byte of the jagged values array
    if (index >= GetSize())
    {
        return FIT_BYTE_INVALID;
    }

    return values[index];
}

FIT_UINT8 FieldBase::GetValuesUINT8(FIT_UINT8 index) const
{
    /// Returns the Nth byte of the jagged values array
    if (index >= GetSize())
    {
        return FIT_UINT8_INVALID;
    }

    return values[index];
}

FIT_SINT8 FieldBase::GetValuesSINT8(FIT_UINT8 index) const
{
    /// Returns the Nth byte of the jagged values array
    if (index >= GetSize())
    {
        return FIT_SINT8_INVALID;
    }

    return values[index];
}

FIT_ENUM FieldBase::GetENUMValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_ENUM>(fieldArrayIndex);
}

FIT_BYTE FieldBase::GetBYTEValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_BYTE>(fieldArrayIndex);
}

FIT_SINT8 FieldBase::GetSINT8Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT8>(fieldArrayIndex);
}

FIT_UINT8 FieldBase::GetUINT8Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT8>(fieldArrayIndex);
}

FIT_UINT8Z FieldBase::GetUINT8ZValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT8Z>(fieldArrayIndex);
}

FIT_SINT16 FieldBase::GetSINT16Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT16>(fieldArrayIndex);
}

FIT_UINT16 FieldBase::GetUINT16Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT16>(fieldArrayIndex);
}

FIT_UINT16Z FieldBase::GetUINT16ZValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT16Z>(fieldArrayIndex);
}

FIT_SINT32 FieldBase::GetSINT32Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT32>(fieldArrayIndex);
}

FIT_UINT32 FieldBase::GetUINT32Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT32Z>(fieldArrayIndex);
}

FIT_UINT32Z FieldBase::GetUINT32ZValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_UINT32Z>(fieldArrayIndex);
}

FIT_SINT64 FieldBase::GetSINT64Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT64>(fieldArrayIndex);
}

FIT_UINT64 FieldBase::GetUINT64Value(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT64>(fieldArrayIndex);
}

FIT_UINT64Z FieldBase::GetUINT64ZValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetValue<FIT_SINT64>(fieldArrayIndex);
}

FIT_FLOAT32 FieldBase::GetFLOAT32Value(const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex) const
{
    FIT_FLOAT32 float32Value;

    if (!IsValid())
        return FIT_FLOAT32_INVALID;

    switch (GetType()) { // Note: This checks the type of the MAIN field since data is aligned according to that type
        case FIT_BASE_TYPE_BYTE:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_BYTE>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_ENUM:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_ENUM>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_SINT8:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_SINT8>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_UINT8:
        case FIT_BASE_TYPE_UINT8Z:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_UINT8>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_SINT16:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_SINT16>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_UINT16:
        case FIT_BASE_TYPE_UINT16Z:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_UINT16>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_SINT32:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_SINT32>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_UINT32:
        case FIT_BASE_TYPE_UINT32Z:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_UINT32>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_SINT64:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_SINT64>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_UINT64:
        case FIT_BASE_TYPE_UINT64Z:
        {
            float32Value =
                ConvertBaseType<FIT_FLOAT32, FIT_UINT64>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT32);
            break;
        }

        case FIT_BASE_TYPE_FLOAT32:
        {
            FIT_SINT32 sint32Value = GetSINT32Value(fieldArrayIndex);

            if (sint32Value == FIT_SINT32_INVALID)
                return FIT_FLOAT32_INVALID;

            memcpy(&float32Value, &sint32Value, sizeof(FIT_FLOAT32));
            break;
        }

        case FIT_BASE_TYPE_FLOAT64:
            return static_cast<FIT_FLOAT32>(GetFLOAT64Value(fieldArrayIndex));

        default:
            return FIT_FLOAT32_INVALID;
    }

    if (memcmp(&float32Value, baseTypeInvalids[FIT_BASE_TYPE_FLOAT32 & FIT_BASE_TYPE_NUM_MASK], sizeof(FIT_FLOAT32)) == 0)
    {
        return float32Value;
    }

    return static_cast<FIT_FLOAT32>(static_cast<FIT_FLOAT64>(float32Value) / GetScale(subFieldIndex) - GetOffset(subFieldIndex));
}

FIT_FLOAT64 FieldBase::GetFLOAT64Value(const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex) const
{
    FIT_FLOAT64 float64Value;

    if (!IsValid())
        return FIT_FLOAT64_INVALID;

    switch (GetType()) { // Note: This checks the type of the MAIN field since data is aligned according to that type
        case FIT_BASE_TYPE_BYTE:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_BYTE>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_ENUM:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_ENUM>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_SINT8:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_SINT8>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_UINT8:
        case FIT_BASE_TYPE_UINT8Z:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_UINT8>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_SINT16:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_SINT16>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_UINT16Z:
        case FIT_BASE_TYPE_UINT16:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_UINT16>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_SINT32:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_SINT32>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_UINT32:
        case FIT_BASE_TYPE_UINT32Z:
        {
            float64Value =
                ConvertBaseType<FIT_FLOAT64, FIT_UINT32>(fieldArrayIndex, FIT_BASE_TYPE_FLOAT64);
            break;
        }

        case FIT_BASE_TYPE_FLOAT32:
        {
            FIT_FLOAT32 val = GetFLOAT32Value(fieldArrayIndex, subFieldIndex);

            if (FIT_FLOAT32_INVALID == val)
            {
                return FIT_FLOAT64_INVALID;
            }

            return static_cast<FIT_FLOAT64>(val);
        }

        case FIT_BASE_TYPE_FLOAT64:
        {
            FIT_UINT64 uint64Value = GetValue<FIT_UINT64>(fieldArrayIndex);
            memcpy(&float64Value, &uint64Value, sizeof(FIT_FLOAT64));
            break;
        }

        default:
            return FIT_FLOAT64_INVALID;
    }

    if (memcmp(&float64Value, baseTypeInvalids[FIT_BASE_TYPE_FLOAT64 & FIT_BASE_TYPE_NUM_MASK], sizeof(FIT_FLOAT64)) == 0)
    {
        return float64Value;
    }

    return float64Value / GetScale(subFieldIndex) - GetOffset(subFieldIndex);
}

FIT_WSTRING FieldBase::GetSTRINGValue(const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex) const
{
    Unicode::UTF8_STRING value;

    if (!IsValid())
        return FIT_WSTRING_INVALID;

    if (GetType() == FIT_BASE_TYPE_STRING) // Note: This checks the type of the MAIN field since data is aligned according to that type
    {
        FIT_UINT8 numStrings = (FIT_UINT8)stringIndexes.size();

        if (numStrings == 0)
            return FIT_WSTRING_INVALID;

        FIT_UINT8 i = 0;
        // Get the start position of the string in the byte array
        FIT_UINT8 index = stringIndexes[fieldArrayIndex];
        FIT_UINT8 length;

        // If this is the last string, its length is the size of the array subtracted
        // by its start position.
        if (fieldArrayIndex + 1 == numStrings)
        {
            length = (FIT_UINT8)(values.size() - index);
        }
        // Otherwise it is the distance between its start position and the start
        // position of the next string in the byte array.
        else
        {
            length = stringIndexes[fieldArrayIndex + 1] - index;
        }

        while ((i < length) && (values[index + i] != 0))
        {
            value += static_cast<Unicode::UTF8_STRING::value_type>(values[index + i]);
            i++;
        }
    }
    else
    {
        FIT_FLOAT64 floatValue = GetFLOAT64Value(fieldArrayIndex, subFieldIndex);

        if (floatValue != FIT_FLOAT64_INVALID)
        {
            Unicode::UTF8_OSSTREAM valueStream;
            valueStream.setf(std::ios_base::fixed);
            valueStream.precision(9);
            valueStream << floatValue;
            value = valueStream.str();

            if ((value.find('.') != Unicode::UTF8_STRING::npos) && (value[value.length() - 1] == '0'))
            {
                Unicode::UTF8_STRING::size_type lastZeroIndex = value.length() - 1;

                while (value[lastZeroIndex - 1] == '0')
                    lastZeroIndex--;

                if (value[lastZeroIndex - 1] == '.')
                    value.erase(lastZeroIndex - 1);
                else
                    value.erase(lastZeroIndex);
            }
        }
        else
        {
            value = "";
        }
    }

    return Unicode::Encode_UTF8ToBase(value);
}

FIT_UINT16 FieldBase::GetSubField(const std::string& subFieldName) const
{
    for (FIT_UINT16 i = 0; i < this->GetNumSubFields(); i++) {
        if ( this->GetSubField(i)->name.compare(subFieldName) == 0)
            return i;
    }

    return FIT_SUBFIELD_INDEX_MAIN_FIELD;
}

FIT_FLOAT64 FieldBase::GetRawValue() const
{
    return GetRawValueInternal(0);
}

FIT_FLOAT64 FieldBase::GetRawValue(const FIT_UINT8 fieldArrayIndex) const
{
    return GetRawValueInternal(fieldArrayIndex);
}

FIT_BOOL FieldBase::GetMemoryValue( const FIT_UINT8 fieldArrayIndex, FIT_UINT8* buffer, const FIT_UINT8 bufferSize ) const
{
    FIT_UINT8 baseTypeSize = baseTypeSizes[GetType() & FIT_BASE_TYPE_NUM_MASK];
    FIT_UINT8 offsetIndex = baseTypeSize * fieldArrayIndex;
    if ( bufferSize < baseTypeSize )
    {
        // Buffer is not large enough
        return FIT_FALSE;
    }

    if ( static_cast<FIT_UINT32>(offsetIndex + baseTypeSize) > values.size() )
    {
        // Values do not contain valid Data
        return FIT_FALSE;
    }

    for ( FIT_UINT8 i = 0; i < baseTypeSize; i++ )
    {
        buffer[i] = values[offsetIndex + i];
    }

    return FIT_TRUE;
}

FIT_FLOAT64 FieldBase::GetRawValueInternal(const FIT_UINT8 fieldArrayIndex) const
{
    FIT_FLOAT64 float64Value;

    if (!IsValid())
        return FIT_FLOAT64_INVALID;

    switch (GetType()) { // Note: This checks the type of the MAIN field since data is aligned according to that type
        case FIT_BASE_TYPE_BYTE:
        {
            FIT_BYTE byteValue = GetBYTEValue(fieldArrayIndex);

            if (byteValue == FIT_BYTE_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = byteValue;
            break;
        }

        case FIT_BASE_TYPE_ENUM:
        {
            FIT_ENUM enumValue = GetENUMValue(fieldArrayIndex);

            if (enumValue == FIT_ENUM_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = enumValue;
            break;
        }

        case FIT_BASE_TYPE_SINT8:
        {
            FIT_SINT8 sint8Value = GetSINT8Value(fieldArrayIndex);

            if (sint8Value == FIT_SINT8_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = sint8Value;
            break;
        }

        case FIT_BASE_TYPE_UINT8:
        {
            FIT_UINT8 uint8Value = GetUINT8Value(fieldArrayIndex);

            if (uint8Value == FIT_UINT8_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint8Value;
            break;
        }

        case FIT_BASE_TYPE_UINT8Z:
        {
            FIT_UINT8Z uint8zValue = GetUINT8ZValue(fieldArrayIndex);

            if (uint8zValue == FIT_UINT8Z_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint8zValue;
            break;
        }

        case FIT_BASE_TYPE_SINT16:
        {
            FIT_SINT16 sint16Value = GetSINT16Value(fieldArrayIndex);

            if (sint16Value == FIT_SINT16_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = sint16Value;
            break;
        }

        case FIT_BASE_TYPE_UINT16:
        {
            FIT_UINT16 uint16Value = GetUINT16Value(fieldArrayIndex);

            if (uint16Value == FIT_UINT16_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint16Value;
            break;
        }

        case FIT_BASE_TYPE_UINT16Z:
        {
            FIT_UINT16Z uint16zValue = GetUINT16ZValue(fieldArrayIndex);

            if (uint16zValue == FIT_UINT16Z_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint16zValue;
            break;
        }

        case FIT_BASE_TYPE_SINT32:
        {
            FIT_SINT32 sint32Value = GetSINT32Value(fieldArrayIndex);

            if (sint32Value == FIT_SINT32_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = sint32Value;
            break;
        }

        case FIT_BASE_TYPE_UINT32:
        {
            FIT_UINT32 uint32Value = GetUINT32Value(fieldArrayIndex);

            if (uint32Value == FIT_UINT32_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint32Value;
            break;
        }

        case FIT_BASE_TYPE_UINT32Z:
        {
            FIT_UINT32Z uint32zValue = GetUINT32ZValue(fieldArrayIndex);

            if (uint32zValue == FIT_UINT32Z_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = uint32zValue;
            break;
        }

        case FIT_BASE_TYPE_FLOAT32:
        {
            FIT_FLOAT32 float32Value = GetFLOAT32Value(fieldArrayIndex);

            if (float32Value == FIT_FLOAT32_INVALID)
                return FIT_FLOAT64_INVALID;

            float64Value = float32Value;
            break;
        }

        case FIT_BASE_TYPE_FLOAT64:
        {
            unsigned long long uint64Value = 0;

            if (fieldArrayIndex >= (FIT_UINT8)values.size())
                return FIT_FLOAT64_INVALID;

            for (size_t i = 0; i < sizeof(FIT_UINT64); i++)
            {
                uint64Value |= ( values[(fieldArrayIndex * sizeof(FIT_UINT64)) + i] << (i * 8) );
            }

            memcpy(&float64Value, &uint64Value, sizeof(FIT_FLOAT64));
            break;
        }

        default:
            return FIT_FLOAT64_INVALID;
    }

    return float64Value;
}

void FieldBase::AddValue(const FIT_FLOAT64 value, const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex)
{
    SetFLOAT64Value( value, fieldArrayIndex, subFieldIndex );
}

// rawValue is already the correct quantity (scale/offsets applied) but possibly not the
// correct underlying type
void FieldBase::AddRawValue(const FIT_FLOAT64 rawValue, const FIT_UINT8 fieldArrayIndex)
{
    FIT_FLOAT64 roundedValue = FieldBase::Round(rawValue);
    switch ( GetType() )
    {
        case FIT_BASE_TYPE_BYTE:
        case FIT_BASE_TYPE_ENUM:
        case FIT_BASE_TYPE_SINT8:
        case FIT_BASE_TYPE_UINT8:
        case FIT_BASE_TYPE_UINT8Z:
            SetUINT8Value((FIT_UINT8)roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_SINT16:
        case FIT_BASE_TYPE_UINT16:
        case FIT_BASE_TYPE_UINT16Z:
            SetUINT16Value((FIT_UINT16)roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_SINT32:
        case FIT_BASE_TYPE_UINT32:
        case FIT_BASE_TYPE_UINT32Z:
            SetUINT32Value((FIT_UINT32)roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_FLOAT32:
        {
            FIT_FLOAT32 float32Value = (FIT_FLOAT32)roundedValue;
            FIT_UINT32 uint32Value;
            memcpy(&uint32Value, &float32Value, sizeof(FIT_FLOAT32));
            SetUINT32Value(uint32Value, fieldArrayIndex);
            break;
        }

        case FIT_BASE_TYPE_FLOAT64:
            if ((FIT_UINT8)values.size() < ((fieldArrayIndex + 1) * sizeof(FIT_FLOAT64)))
            {
                values.resize((fieldArrayIndex + 1) * 8);
            }

            for (size_t i = 0; i < sizeof(FIT_FLOAT64); i++)
            {
                values[(fieldArrayIndex * sizeof(FIT_FLOAT64) + i)] = (*(((FIT_BYTE *)&rawValue) + i));
            }
            break;

        default:
            break;
    }
}


void FieldBase::SetENUMValue(const FIT_ENUM value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT8Value(value, fieldArrayIndex);
}

void FieldBase::SetBYTEValue(const FIT_BYTE value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT8Value(value, fieldArrayIndex);
}

void FieldBase::SetSINT8Value(const FIT_SINT8 value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT8Value(value, fieldArrayIndex);
}

void FieldBase::SetUINT8Value(const FIT_UINT8 value, const FIT_UINT8 fieldArrayIndex)
{
    if ((FIT_UINT8)values.size() < (fieldArrayIndex + 1))
    {
        values.resize(fieldArrayIndex + 1);
    }

    values[fieldArrayIndex] = (FIT_BYTE)value;
}

void FieldBase::SetUINT8ZValue(const FIT_UINT8 value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT8Value(value, fieldArrayIndex);
}

void FieldBase::SetSINT16Value(const FIT_SINT16 value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT16Value(value, fieldArrayIndex);
}

void FieldBase::SetUINT16Value(const FIT_UINT16 value, const FIT_UINT8 fieldArrayIndex)
{
    if ((FIT_UINT8)values.size() < ((fieldArrayIndex + 1) * sizeof(FIT_UINT16)))
    {
        values.resize((fieldArrayIndex + 1) * sizeof(FIT_UINT16));
    }

    for (size_t i = 0; i < sizeof(FIT_UINT16); i++)
    {
        values[(fieldArrayIndex * sizeof(FIT_UINT16)) + i] = ((FIT_BYTE)(value >> (8 * i)));
    }
}

void FieldBase::SetUINT16ZValue(const FIT_UINT16Z value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT16Value(value, fieldArrayIndex);
}

void FieldBase::SetSINT32Value(const FIT_SINT32 value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT32Value(value, fieldArrayIndex);
}

void FieldBase::SetUINT32Value(const FIT_UINT32 value, const FIT_UINT8 fieldArrayIndex)
{
    if ((FIT_UINT8)values.size() < ((fieldArrayIndex + 1) * sizeof(FIT_UINT32)))
    {
        values.resize((fieldArrayIndex + 1) * sizeof(FIT_UINT32));
    }

    for (size_t i = 0; i < sizeof(FIT_UINT32); i++)
    {
        values[(fieldArrayIndex * sizeof(FIT_UINT32)) + i] = ((FIT_BYTE)(value >> (8 * i)));
    }
}

void FieldBase::SetUINT32ZValue(const FIT_UINT32Z value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT32Value(value, fieldArrayIndex);
}

void FieldBase::SetSINT64Value(const FIT_SINT64 value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT64Value(value, fieldArrayIndex);
}

void FieldBase::SetUINT64Value(const FIT_UINT64 value, const FIT_UINT8 fieldArrayIndex)
{
    if ((FIT_UINT8)values.size() < ((fieldArrayIndex + 1) * sizeof(FIT_UINT64)))
    {
        values.resize((fieldArrayIndex + 1) * sizeof(FIT_UINT64));
    }

    for (size_t i = 0; i < sizeof(FIT_UINT64); i++)
    {
        values[(fieldArrayIndex * sizeof(FIT_UINT64)) + i] = ((FIT_BYTE)(value >> (8 * i)));
    }
}

void FieldBase::SetUINT64ZValue(const FIT_UINT64Z value, const FIT_UINT8 fieldArrayIndex)
{
    SetUINT64Value(value, fieldArrayIndex);
}

void FieldBase::SetFLOAT32Value(const FIT_FLOAT32 value, const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex)
{
    SetFLOAT64Value(value, fieldArrayIndex, subFieldIndex);
}

void FieldBase::SetFLOAT64Value(const FIT_FLOAT64 value, const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subFieldIndex)
{
    if (!IsValid())
        return;

    FIT_FLOAT64 recalculatedValue = (value + GetOffset(subFieldIndex)) * GetScale(subFieldIndex);
    // Make sure floating point representations trunc to the expected integer
    FIT_FLOAT64 roundedValue = (recalculatedValue >= 0.0) ? (recalculatedValue + 0.5) : (recalculatedValue - 0.5);

    switch (GetType()) { // Note: This checks the type of the MAIN field since data is aligned according to that type
        case FIT_BASE_TYPE_BYTE:
        case FIT_BASE_TYPE_ENUM:
        case FIT_BASE_TYPE_SINT8:
        case FIT_BASE_TYPE_UINT8:
        case FIT_BASE_TYPE_UINT8Z:
            SetUINT8Value((FIT_UINT8) roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_SINT16:
        case FIT_BASE_TYPE_UINT16:
        case FIT_BASE_TYPE_UINT16Z:
            SetUINT16Value((FIT_UINT16) roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_SINT32:
        case FIT_BASE_TYPE_UINT32:
        case FIT_BASE_TYPE_UINT32Z:
            SetUINT32Value((FIT_UINT32) roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_SINT64:
        case FIT_BASE_TYPE_UINT64:
        case FIT_BASE_TYPE_UINT64Z:
            SetUINT64Value((FIT_UINT64) roundedValue, fieldArrayIndex);
            break;

        case FIT_BASE_TYPE_FLOAT32:
        {
            FIT_FLOAT32 float32Value = (FIT_FLOAT32)recalculatedValue;
            FIT_UINT32 uint32Value;
            memcpy(&uint32Value, &float32Value, sizeof(FIT_FLOAT32));
            SetUINT32Value(uint32Value, fieldArrayIndex);
            break;
        }

        case FIT_BASE_TYPE_FLOAT64:
            if ((FIT_UINT8)values.size() < ((fieldArrayIndex + 1) * sizeof(FIT_FLOAT64)))
            {
                values.resize((fieldArrayIndex + 1) * sizeof(FIT_FLOAT64));
            }

            for (size_t i = 0; i < sizeof(FIT_FLOAT64); i++)
            {
                values[(fieldArrayIndex * sizeof(FIT_FLOAT64)) + i] = *(((FIT_BYTE *)&recalculatedValue) + i);
            }

            break;

        default:
            break;
    }
}

void FieldBase::SetSTRINGValue(const FIT_WSTRING& value, const FIT_UINT8 fieldArrayIndex)
{
    if (!IsValid())
        return;

    Unicode::UTF8_STRING stringValue = Unicode::Encode_BaseToUTF8(value);
    FIT_UINT8 stringLength = (FIT_UINT8)stringValue.length();
    FIT_UINT8 idx = 0;
    FIT_UINT8 nextIdx = 0;
    FIT_UINT8 numStrings = (FIT_UINT8)stringIndexes.size();
    FIT_UINT8 dif = 0;

    if ( fieldArrayIndex < numStrings )
    {
        idx = stringIndexes[fieldArrayIndex];
        if (fieldArrayIndex + 1 < numStrings)
        {
            nextIdx = stringIndexes[fieldArrayIndex + 1];
            dif = (FIT_UINT8)( (value.length() + 1) - (nextIdx - idx) );
            values.erase(values.begin() + idx, values.begin() + nextIdx);
        }
        else
        {
            values.erase(values.begin() + idx, values.end());
        }

        for (auto i = 0; i < stringLength + 1; i++)
            values.insert(values.begin() + idx + i, 0);
    }
    else
    {
        idx = (FIT_UINT8)values.size();
        numStrings++;
        stringIndexes.push_back(idx);
        for (auto j = 0; j < stringLength + 1; j++)
        {
            values.push_back(0);
        }
    }

    int i;
    for (i = 0; i < (int)stringValue.length(); i++)
    {
        values[idx + i] = static_cast<FIT_BYTE>(stringValue[i]);
    }

    values[idx + i] = 0; // null terminate
}

FIT_BOOL FieldBase::Read(const void *data, const FIT_UINT8 size)
{
    FIT_UINT8 bytesLeft = size;
    FIT_UINT8 typeSize = baseTypeSizes[GetType() & FIT_BASE_TYPE_NUM_MASK];
    FIT_BYTE *byteData = (FIT_BYTE *) data;

    values.clear();

    switch (GetType())
    {
        case FIT_BASE_TYPE_STRING:
        {
            FIT_BOOL hasData = FIT_FALSE;
            FIT_UINT8 stringPos = 0;
            FIT_UINT8 index = 0;

            // Check if the string is empty
            while (index < bytesLeft)
            {
                if (byteData[index] != 0)
                {
                    hasData = FIT_TRUE;
                    break;
                }
                index++;
            }

            if (!hasData)
                break;

            stringIndexes.push_back(stringPos);

            while (bytesLeft-- > 0)
            {
                FIT_BYTE byte = *byteData++;
                values.push_back(byte);
                stringPos++;

                if ( ( byte == 0 ) && ( stringPos + 1 < size ) )
                {
                    stringIndexes.push_back(stringPos);
                }
            }

            // Null terminate if not already null terminated
            if ( values.back() != 0 )
            {
                values.push_back(0);
            }
        }
            break;

        default:
        {
            int bytes = bytesLeft;
            FIT_BYTE *point = byteData;

            while (bytes > 0)
            {
                if (memcmp(point, baseTypeInvalids[GetType() & FIT_BASE_TYPE_NUM_MASK], typeSize) != 0)
                {
                    values.insert(values.end(), byteData, byteData + bytesLeft);
                    break;
                }
                point += typeSize;
                bytes -= typeSize;
            }
        }
            break;
    }

    return FIT_TRUE;
}

FIT_UINT8 FieldBase::Write(std::ostream &file) const
{
    file.write((const char*)values.data(), values.size());

    return GetSize();
}

FIT_BOOL FieldBase::IsValueValid( const FIT_UINT8 fieldArrayIndex, const FIT_UINT16 subfieldIndex ) const
{
    FIT_BOOL isValid = FIT_FALSE;

    FIT_UINT8 type = GetType( subfieldIndex );

    if ( FIT_UINT8_INVALID == type )
    {
        // Invalid Subfield Index use the default type
        type = GetType();
    }

    if ( FIT_BASE_TYPE_STRING == type )
    {
        FIT_WSTRING strVal = GetSTRINGValue( fieldArrayIndex );

        isValid = ( strVal != FIT_WSTRING_INVALID );
    }
    else
    {
        FIT_UINT8 baseTypeSize = baseTypeSizes[type & FIT_BASE_TYPE_NUM_MASK];
        const FIT_UINT8* invalid = baseTypeInvalids[type & FIT_BASE_TYPE_NUM_MASK];
        FIT_UINT8* data = new FIT_UINT8[baseTypeSize];

        FIT_BOOL readSuccess = GetMemoryValue( fieldArrayIndex, data, baseTypeSize );

        //! TODO Handle Strings
        if ( readSuccess )
        {
            isValid = ( memcmp( invalid, data, baseTypeSize ) != 0 );
        }
        delete[] data;
    }

    return isValid;
}

FIT_FLOAT64 FieldBase::Round(FIT_FLOAT64 value)
{
    return (value > 0.0) ? floor(value + 0.5) : ceil(value - 0.5);
}

template <typename T>
T FieldBase::GetValue(const FIT_UINT8 fieldArrayIndex) const
{
    FIT_UINT8 size = baseTypeSizes[GetType() & FIT_BASE_TYPE_NUM_MASK];
    if (fieldArrayIndex >= (FIT_UINT8)values.size())
    {
        return *reinterpret_cast<const T*>(baseTypeInvalids[GetType() & FIT_BASE_TYPE_NUM_MASK]);
    }

    T rv = 0;

    for (int i = 0; i < size; i++)
    {
        rv |= static_cast<T>(values[(fieldArrayIndex * size) + i]) << (i * 8);
    }

    return rv;
}


template <typename TTo, typename TFrom>
TTo FieldBase::ConvertBaseType(const FIT_UINT8 fieldArrayIndex, const FIT_UINT8 toBaseType) const
{
    FIT_CONST_UINT8_PTR invalid = baseTypeInvalids[GetType() & FIT_BASE_TYPE_NUM_MASK];
    TFrom value = GetValue<TFrom>(fieldArrayIndex);

    if (memcmp(&value, invalid, sizeof(TFrom)) == 0)
        return *reinterpret_cast<const TTo*>(baseTypeInvalids[toBaseType & FIT_BASE_TYPE_NUM_MASK]);

    return static_cast<TTo>(value);
}
} // namespace fit
