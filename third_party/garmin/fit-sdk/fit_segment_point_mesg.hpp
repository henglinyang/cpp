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


#if !defined(FIT_SEGMENT_POINT_MESG_HPP)
#define FIT_SEGMENT_POINT_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class SegmentPointMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 MessageIndex = 254;
       static const FIT_UINT8 PositionLat = 1;
       static const FIT_UINT8 PositionLong = 2;
       static const FIT_UINT8 Distance = 3;
       static const FIT_UINT8 Altitude = 4;
       static const FIT_UINT8 LeaderTime = 5;
       static const FIT_UINT8 EnhancedAltitude = 6;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    SegmentPointMesg(void) : Mesg(Profile::MESG_SEGMENT_POINT)
    {
    }

    SegmentPointMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of message_index field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsMessageIndexValid() const
    {
        const Field* field = GetField(254);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns message_index field
    ///////////////////////////////////////////////////////////////////////
    FIT_MESSAGE_INDEX GetMessageIndex(void) const
    {
        return GetFieldUINT16Value(254, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set message_index field
    ///////////////////////////////////////////////////////////////////////
    void SetMessageIndex(FIT_MESSAGE_INDEX messageIndex)
    {
        SetFieldUINT16Value(254, messageIndex, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of position_lat field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsPositionLatValid() const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns position_lat field
    // Units: semicircles
    ///////////////////////////////////////////////////////////////////////
    FIT_SINT32 GetPositionLat(void) const
    {
        return GetFieldSINT32Value(1, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set position_lat field
    // Units: semicircles
    ///////////////////////////////////////////////////////////////////////
    void SetPositionLat(FIT_SINT32 positionLat)
    {
        SetFieldSINT32Value(1, positionLat, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of position_long field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsPositionLongValid() const
    {
        const Field* field = GetField(2);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns position_long field
    // Units: semicircles
    ///////////////////////////////////////////////////////////////////////
    FIT_SINT32 GetPositionLong(void) const
    {
        return GetFieldSINT32Value(2, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set position_long field
    // Units: semicircles
    ///////////////////////////////////////////////////////////////////////
    void SetPositionLong(FIT_SINT32 positionLong)
    {
        SetFieldSINT32Value(2, positionLong, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of distance field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsDistanceValid() const
    {
        const Field* field = GetField(3);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns distance field
    // Units: m
    // Comment: Accumulated distance along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetDistance(void) const
    {
        return GetFieldFLOAT32Value(3, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set distance field
    // Units: m
    // Comment: Accumulated distance along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    void SetDistance(FIT_FLOAT32 distance)
    {
        SetFieldFLOAT32Value(3, distance, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of altitude field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsAltitudeValid() const
    {
        const Field* field = GetField(4);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns altitude field
    // Units: m
    // Comment: Accumulated altitude along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetAltitude(void) const
    {
        return GetFieldFLOAT32Value(4, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set altitude field
    // Units: m
    // Comment: Accumulated altitude along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    void SetAltitude(FIT_FLOAT32 altitude)
    {
        SetFieldFLOAT32Value(4, altitude, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of leader_time
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumLeaderTime(void) const
    {
        return GetFieldNumValues(5, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of leader_time field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsLeaderTimeValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(5);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns leader_time field
    // Units: s
    // Comment: Accumualted time each leader board member required to reach the described point. This value is zero for all leader board members at the starting point of the segment.
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetLeaderTime(FIT_UINT8 index) const
    {
        return GetFieldFLOAT32Value(5, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set leader_time field
    // Units: s
    // Comment: Accumualted time each leader board member required to reach the described point. This value is zero for all leader board members at the starting point of the segment.
    ///////////////////////////////////////////////////////////////////////
    void SetLeaderTime(FIT_UINT8 index, FIT_FLOAT32 leaderTime)
    {
        SetFieldFLOAT32Value(5, leaderTime, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of enhanced_altitude field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsEnhancedAltitudeValid() const
    {
        const Field* field = GetField(6);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns enhanced_altitude field
    // Units: m
    // Comment: Accumulated altitude along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    FIT_FLOAT32 GetEnhancedAltitude(void) const
    {
        return GetFieldFLOAT32Value(6, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set enhanced_altitude field
    // Units: m
    // Comment: Accumulated altitude along the segment at the described point
    ///////////////////////////////////////////////////////////////////////
    void SetEnhancedAltitude(FIT_FLOAT32 enhancedAltitude)
    {
        SetFieldFLOAT32Value(6, enhancedAltitude, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_SEGMENT_POINT_MESG_HPP)
