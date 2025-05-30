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


#if !defined(FIT_FIELD_DESCRIPTION_MESG_HPP)
#define FIT_FIELD_DESCRIPTION_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class FieldDescriptionMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 DeveloperDataIndex = 0;
       static const FIT_UINT8 FieldDefinitionNumber = 1;
       static const FIT_UINT8 FitBaseTypeId = 2;
       static const FIT_UINT8 FieldName = 3;
       static const FIT_UINT8 Array = 4;
       static const FIT_UINT8 Components = 5;
       static const FIT_UINT8 Scale = 6;
       static const FIT_UINT8 Offset = 7;
       static const FIT_UINT8 Units = 8;
       static const FIT_UINT8 Bits = 9;
       static const FIT_UINT8 Accumulate = 10;
       static const FIT_UINT8 FitBaseUnitId = 13;
       static const FIT_UINT8 NativeMesgNum = 14;
       static const FIT_UINT8 NativeFieldNum = 15;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    FieldDescriptionMesg(void) : Mesg(Profile::MESG_FIELD_DESCRIPTION)
    {
    }

    FieldDescriptionMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of developer_data_index field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsDeveloperDataIndexValid() const
    {
        const Field* field = GetField(0);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns developer_data_index field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetDeveloperDataIndex(void) const
    {
        return GetFieldUINT8Value(0, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set developer_data_index field
    ///////////////////////////////////////////////////////////////////////
    void SetDeveloperDataIndex(FIT_UINT8 developerDataIndex)
    {
        SetFieldUINT8Value(0, developerDataIndex, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of field_definition_number field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsFieldDefinitionNumberValid() const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns field_definition_number field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetFieldDefinitionNumber(void) const
    {
        return GetFieldUINT8Value(1, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set field_definition_number field
    ///////////////////////////////////////////////////////////////////////
    void SetFieldDefinitionNumber(FIT_UINT8 fieldDefinitionNumber)
    {
        SetFieldUINT8Value(1, fieldDefinitionNumber, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of fit_base_type_id field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsFitBaseTypeIdValid() const
    {
        const Field* field = GetField(2);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns fit_base_type_id field
    ///////////////////////////////////////////////////////////////////////
    FIT_FIT_BASE_TYPE GetFitBaseTypeId(void) const
    {
        return GetFieldUINT8Value(2, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set fit_base_type_id field
    ///////////////////////////////////////////////////////////////////////
    void SetFitBaseTypeId(FIT_FIT_BASE_TYPE fitBaseTypeId)
    {
        SetFieldUINT8Value(2, fitBaseTypeId, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of field_name
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumFieldName(void) const
    {
        return GetFieldNumValues(3, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of field_name field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsFieldNameValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(3);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns field_name field
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetFieldName(FIT_UINT8 index) const
    {
        return GetFieldSTRINGValue(3, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set field_name field
    ///////////////////////////////////////////////////////////////////////
    void SetFieldName(FIT_UINT8 index, FIT_WSTRING fieldName)
    {
        SetFieldSTRINGValue(3, fieldName, index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of array field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsArrayValid() const
    {
        const Field* field = GetField(4);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns array field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetArray(void) const
    {
        return GetFieldUINT8Value(4, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set array field
    ///////////////////////////////////////////////////////////////////////
    void SetArray(FIT_UINT8 array)
    {
        SetFieldUINT8Value(4, array, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of components field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsComponentsValid() const
    {
        const Field* field = GetField(5);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns components field
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetComponents(void) const
    {
        return GetFieldSTRINGValue(5, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set components field
    ///////////////////////////////////////////////////////////////////////
    void SetComponents(FIT_WSTRING components)
    {
        SetFieldSTRINGValue(5, components, 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of scale field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsScaleValid() const
    {
        const Field* field = GetField(6);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns scale field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetScale(void) const
    {
        return GetFieldUINT8Value(6, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set scale field
    ///////////////////////////////////////////////////////////////////////
    void SetScale(FIT_UINT8 scale)
    {
        SetFieldUINT8Value(6, scale, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of offset field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsOffsetValid() const
    {
        const Field* field = GetField(7);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns offset field
    ///////////////////////////////////////////////////////////////////////
    FIT_SINT8 GetOffset(void) const
    {
        return GetFieldSINT8Value(7, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set offset field
    ///////////////////////////////////////////////////////////////////////
    void SetOffset(FIT_SINT8 offset)
    {
        SetFieldSINT8Value(7, offset, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of units
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumUnits(void) const
    {
        return GetFieldNumValues(8, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of units field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsUnitsValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(8);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns units field
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetUnits(FIT_UINT8 index) const
    {
        return GetFieldSTRINGValue(8, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set units field
    ///////////////////////////////////////////////////////////////////////
    void SetUnits(FIT_UINT8 index, FIT_WSTRING units)
    {
        SetFieldSTRINGValue(8, units, index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of bits field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsBitsValid() const
    {
        const Field* field = GetField(9);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns bits field
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetBits(void) const
    {
        return GetFieldSTRINGValue(9, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set bits field
    ///////////////////////////////////////////////////////////////////////
    void SetBits(FIT_WSTRING bits)
    {
        SetFieldSTRINGValue(9, bits, 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of accumulate field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsAccumulateValid() const
    {
        const Field* field = GetField(10);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns accumulate field
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetAccumulate(void) const
    {
        return GetFieldSTRINGValue(10, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set accumulate field
    ///////////////////////////////////////////////////////////////////////
    void SetAccumulate(FIT_WSTRING accumulate)
    {
        SetFieldSTRINGValue(10, accumulate, 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of fit_base_unit_id field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsFitBaseUnitIdValid() const
    {
        const Field* field = GetField(13);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns fit_base_unit_id field
    ///////////////////////////////////////////////////////////////////////
    FIT_FIT_BASE_UNIT GetFitBaseUnitId(void) const
    {
        return GetFieldUINT16Value(13, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set fit_base_unit_id field
    ///////////////////////////////////////////////////////////////////////
    void SetFitBaseUnitId(FIT_FIT_BASE_UNIT fitBaseUnitId)
    {
        SetFieldUINT16Value(13, fitBaseUnitId, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of native_mesg_num field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsNativeMesgNumValid() const
    {
        const Field* field = GetField(14);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns native_mesg_num field
    ///////////////////////////////////////////////////////////////////////
    FIT_MESG_NUM GetNativeMesgNum(void) const
    {
        return GetFieldUINT16Value(14, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set native_mesg_num field
    ///////////////////////////////////////////////////////////////////////
    void SetNativeMesgNum(FIT_MESG_NUM nativeMesgNum)
    {
        SetFieldUINT16Value(14, nativeMesgNum, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of native_field_num field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsNativeFieldNumValid() const
    {
        const Field* field = GetField(15);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns native_field_num field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNativeFieldNum(void) const
    {
        return GetFieldUINT8Value(15, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set native_field_num field
    ///////////////////////////////////////////////////////////////////////
    void SetNativeFieldNum(FIT_UINT8 nativeFieldNum)
    {
        SetFieldUINT8Value(15, nativeFieldNum, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_FIELD_DESCRIPTION_MESG_HPP)
