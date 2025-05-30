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


#if !defined(FIT_SEGMENT_ID_MESG_HPP)
#define FIT_SEGMENT_ID_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class SegmentIdMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 Name = 0;
       static const FIT_UINT8 Uuid = 1;
       static const FIT_UINT8 Sport = 2;
       static const FIT_UINT8 Enabled = 3;
       static const FIT_UINT8 UserProfilePrimaryKey = 4;
       static const FIT_UINT8 DeviceId = 5;
       static const FIT_UINT8 DefaultRaceLeader = 6;
       static const FIT_UINT8 DeleteStatus = 7;
       static const FIT_UINT8 SelectionType = 8;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    SegmentIdMesg(void) : Mesg(Profile::MESG_SEGMENT_ID)
    {
    }

    SegmentIdMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of name field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsNameValid() const
    {
        const Field* field = GetField(0);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns name field
    // Comment: Friendly name assigned to segment
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetName(void) const
    {
        return GetFieldSTRINGValue(0, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set name field
    // Comment: Friendly name assigned to segment
    ///////////////////////////////////////////////////////////////////////
    void SetName(FIT_WSTRING name)
    {
        SetFieldSTRINGValue(0, name, 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of uuid field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsUuidValid() const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns uuid field
    // Comment: UUID of the segment
    ///////////////////////////////////////////////////////////////////////
    FIT_WSTRING GetUuid(void) const
    {
        return GetFieldSTRINGValue(1, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set uuid field
    // Comment: UUID of the segment
    ///////////////////////////////////////////////////////////////////////
    void SetUuid(FIT_WSTRING uuid)
    {
        SetFieldSTRINGValue(1, uuid, 0);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of sport field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsSportValid() const
    {
        const Field* field = GetField(2);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns sport field
    // Comment: Sport associated with the segment
    ///////////////////////////////////////////////////////////////////////
    FIT_SPORT GetSport(void) const
    {
        return GetFieldENUMValue(2, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set sport field
    // Comment: Sport associated with the segment
    ///////////////////////////////////////////////////////////////////////
    void SetSport(FIT_SPORT sport)
    {
        SetFieldENUMValue(2, sport, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of enabled field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsEnabledValid() const
    {
        const Field* field = GetField(3);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns enabled field
    // Comment: Segment enabled for evaluation
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL GetEnabled(void) const
    {
        return GetFieldENUMValue(3, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set enabled field
    // Comment: Segment enabled for evaluation
    ///////////////////////////////////////////////////////////////////////
    void SetEnabled(FIT_BOOL enabled)
    {
        SetFieldENUMValue(3, enabled, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of user_profile_primary_key field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsUserProfilePrimaryKeyValid() const
    {
        const Field* field = GetField(4);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns user_profile_primary_key field
    // Comment: Primary key of the user that created the segment
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT32 GetUserProfilePrimaryKey(void) const
    {
        return GetFieldUINT32Value(4, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set user_profile_primary_key field
    // Comment: Primary key of the user that created the segment
    ///////////////////////////////////////////////////////////////////////
    void SetUserProfilePrimaryKey(FIT_UINT32 userProfilePrimaryKey)
    {
        SetFieldUINT32Value(4, userProfilePrimaryKey, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of device_id field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsDeviceIdValid() const
    {
        const Field* field = GetField(5);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns device_id field
    // Comment: ID of the device that created the segment
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT32 GetDeviceId(void) const
    {
        return GetFieldUINT32Value(5, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set device_id field
    // Comment: ID of the device that created the segment
    ///////////////////////////////////////////////////////////////////////
    void SetDeviceId(FIT_UINT32 deviceId)
    {
        SetFieldUINT32Value(5, deviceId, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of default_race_leader field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsDefaultRaceLeaderValid() const
    {
        const Field* field = GetField(6);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns default_race_leader field
    // Comment: Index for the Leader Board entry selected as the default race participant
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetDefaultRaceLeader(void) const
    {
        return GetFieldUINT8Value(6, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set default_race_leader field
    // Comment: Index for the Leader Board entry selected as the default race participant
    ///////////////////////////////////////////////////////////////////////
    void SetDefaultRaceLeader(FIT_UINT8 defaultRaceLeader)
    {
        SetFieldUINT8Value(6, defaultRaceLeader, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of delete_status field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsDeleteStatusValid() const
    {
        const Field* field = GetField(7);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns delete_status field
    // Comment: Indicates if any segments should be deleted
    ///////////////////////////////////////////////////////////////////////
    FIT_SEGMENT_DELETE_STATUS GetDeleteStatus(void) const
    {
        return GetFieldENUMValue(7, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set delete_status field
    // Comment: Indicates if any segments should be deleted
    ///////////////////////////////////////////////////////////////////////
    void SetDeleteStatus(FIT_SEGMENT_DELETE_STATUS deleteStatus)
    {
        SetFieldENUMValue(7, deleteStatus, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of selection_type field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsSelectionTypeValid() const
    {
        const Field* field = GetField(8);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns selection_type field
    // Comment: Indicates how the segment was selected to be sent to the device
    ///////////////////////////////////////////////////////////////////////
    FIT_SEGMENT_SELECTION_TYPE GetSelectionType(void) const
    {
        return GetFieldENUMValue(8, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set selection_type field
    // Comment: Indicates how the segment was selected to be sent to the device
    ///////////////////////////////////////////////////////////////////////
    void SetSelectionType(FIT_SEGMENT_SELECTION_TYPE selectionType)
    {
        SetFieldENUMValue(8, selectionType, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_SEGMENT_ID_MESG_HPP)
