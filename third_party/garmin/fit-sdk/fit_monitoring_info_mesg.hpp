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


#if !defined(FIT_MONITORING_INFO_MESG_HPP)
#define FIT_MONITORING_INFO_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class MonitoringInfoMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 Timestamp = 253;
       static const FIT_UINT8 LocalTimestamp = 0;
       static const FIT_UINT8 ActivityType = 1;
       static const FIT_UINT8 CyclesToDistance = 3;
       static const FIT_UINT8 CyclesToCalories = 4;
       static const FIT_UINT8 RestingMetabolicRate = 5;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    MonitoringInfoMesg(void) : Mesg(Profile::MESG_MONITORING_INFO)
    {
    }

    MonitoringInfoMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of timestamp field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsTimestampValid() const
    {
        const Field* field = GetField(253);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns timestamp field
    // Units: s
    ///////////////////////////////////////////////////////////////////////
    FIT_DATE_TIME GetTimestamp(void) const
    {
        return GetFieldUINT32Value(253, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set timestamp field
    // Units: s
    ///////////////////////////////////////////////////////////////////////
    void SetTimestamp(FIT_DATE_TIME timestamp)
    {
        SetFieldUINT32Value(253, timestamp, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of local_timestamp field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsLocalTimestampValid() const
    {
        const Field* field = GetField(0);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns local_timestamp field
    // Units: s
    // Comment: Use to convert activity timestamps to local time if device does not support time zone and daylight savings time correction.
    ///////////////////////////////////////////////////////////////////////
    FIT_LOCAL_DATE_TIME GetLocalTimestamp(void) const
    {
        return GetFieldUINT32Value(0, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set local_timestamp field
    // Units: s
    // Comment: Use to convert activity timestamps to local time if device does not support time zone and daylight savings time correction.
    ///////////////////////////////////////////////////////////////////////
    void SetLocalTimestamp(FIT_LOCAL_DATE_TIME localTimestamp)
    {
        SetFieldUINT32Value(0, localTimestamp, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of activity_type
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumActivityType(void) const
    {
        return GetFieldNumValues(1, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of activity_type field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsActivityTypeValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns activity_type field
    ///////////////////////////////////////////////////////////////////////
    FIT_ACTIVITY_TYPE GetActivityType(FIT_UINT8 index) const
    {
        return GetFieldENUMValue(1, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set activity_type field
    ///////////////////////////////////////////////////////////////////////
    void SetActivityType(FIT_UINT8 index, FIT_ACTIVITY_TYPE activityType)
    {
        SetFieldENUMValue(1, activityType, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of cycles_to_distance
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumCyclesToDistance(void) const
    {
        return GetFieldNumValues(3, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of cycles_to_distance field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsCyclesToDistanceValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(3);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns cycles_to_distance field
    // Units: m/cycle
    // Comment: Indexed by activity_type
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetCyclesToDistance(FIT_UINT8 index) const
    {
        return GetFieldFLOAT32Value(3, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set cycles_to_distance field
    // Units: m/cycle
    // Comment: Indexed by activity_type
    ///////////////////////////////////////////////////////////////////////
    void SetCyclesToDistance(FIT_UINT8 index, FIT_FLOAT32 cyclesToDistance)
    {
        SetFieldFLOAT32Value(3, cyclesToDistance, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of cycles_to_calories
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumCyclesToCalories(void) const
    {
        return GetFieldNumValues(4, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of cycles_to_calories field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsCyclesToCaloriesValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(4);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns cycles_to_calories field
    // Units: kcal/cycle
    // Comment: Indexed by activity_type
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetCyclesToCalories(FIT_UINT8 index) const
    {
        return GetFieldFLOAT32Value(4, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set cycles_to_calories field
    // Units: kcal/cycle
    // Comment: Indexed by activity_type
    ///////////////////////////////////////////////////////////////////////
    void SetCyclesToCalories(FIT_UINT8 index, FIT_FLOAT32 cyclesToCalories)
    {
        SetFieldFLOAT32Value(4, cyclesToCalories, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of resting_metabolic_rate field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsRestingMetabolicRateValid() const
    {
        const Field* field = GetField(5);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns resting_metabolic_rate field
    // Units: kcal / day
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT16 GetRestingMetabolicRate(void) const
    {
        return GetFieldUINT16Value(5, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set resting_metabolic_rate field
    // Units: kcal / day
    ///////////////////////////////////////////////////////////////////////
    void SetRestingMetabolicRate(FIT_UINT16 restingMetabolicRate)
    {
        SetFieldUINT16Value(5, restingMetabolicRate, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_MONITORING_INFO_MESG_HPP)
