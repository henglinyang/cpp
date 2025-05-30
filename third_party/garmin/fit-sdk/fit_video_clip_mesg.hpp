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


#if !defined(FIT_VIDEO_CLIP_MESG_HPP)
#define FIT_VIDEO_CLIP_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class VideoClipMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 ClipNumber = 0;
       static const FIT_UINT8 StartTimestamp = 1;
       static const FIT_UINT8 StartTimestampMs = 2;
       static const FIT_UINT8 EndTimestamp = 3;
       static const FIT_UINT8 EndTimestampMs = 4;
       static const FIT_UINT8 ClipStart = 6;
       static const FIT_UINT8 ClipEnd = 7;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    VideoClipMesg(void) : Mesg(Profile::MESG_VIDEO_CLIP)
    {
    }

    VideoClipMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of clip_number field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsClipNumberValid() const
    {
        const Field* field = GetField(0);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns clip_number field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT16 GetClipNumber(void) const
    {
        return GetFieldUINT16Value(0, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set clip_number field
    ///////////////////////////////////////////////////////////////////////
    void SetClipNumber(FIT_UINT16 clipNumber)
    {
        SetFieldUINT16Value(0, clipNumber, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of start_timestamp field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsStartTimestampValid() const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns start_timestamp field
    ///////////////////////////////////////////////////////////////////////
    FIT_DATE_TIME GetStartTimestamp(void) const
    {
        return GetFieldUINT32Value(1, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set start_timestamp field
    ///////////////////////////////////////////////////////////////////////
    void SetStartTimestamp(FIT_DATE_TIME startTimestamp)
    {
        SetFieldUINT32Value(1, startTimestamp, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of start_timestamp_ms field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsStartTimestampMsValid() const
    {
        const Field* field = GetField(2);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns start_timestamp_ms field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT16 GetStartTimestampMs(void) const
    {
        return GetFieldUINT16Value(2, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set start_timestamp_ms field
    ///////////////////////////////////////////////////////////////////////
    void SetStartTimestampMs(FIT_UINT16 startTimestampMs)
    {
        SetFieldUINT16Value(2, startTimestampMs, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of end_timestamp field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsEndTimestampValid() const
    {
        const Field* field = GetField(3);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns end_timestamp field
    ///////////////////////////////////////////////////////////////////////
    FIT_DATE_TIME GetEndTimestamp(void) const
    {
        return GetFieldUINT32Value(3, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set end_timestamp field
    ///////////////////////////////////////////////////////////////////////
    void SetEndTimestamp(FIT_DATE_TIME endTimestamp)
    {
        SetFieldUINT32Value(3, endTimestamp, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of end_timestamp_ms field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsEndTimestampMsValid() const
    {
        const Field* field = GetField(4);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns end_timestamp_ms field
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT16 GetEndTimestampMs(void) const
    {
        return GetFieldUINT16Value(4, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set end_timestamp_ms field
    ///////////////////////////////////////////////////////////////////////
    void SetEndTimestampMs(FIT_UINT16 endTimestampMs)
    {
        SetFieldUINT16Value(4, endTimestampMs, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of clip_start field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsClipStartValid() const
    {
        const Field* field = GetField(6);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns clip_start field
    // Units: ms
    // Comment: Start of clip in video time
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT32 GetClipStart(void) const
    {
        return GetFieldUINT32Value(6, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set clip_start field
    // Units: ms
    // Comment: Start of clip in video time
    ///////////////////////////////////////////////////////////////////////
    void SetClipStart(FIT_UINT32 clipStart)
    {
        SetFieldUINT32Value(6, clipStart, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of clip_end field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsClipEndValid() const
    {
        const Field* field = GetField(7);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns clip_end field
    // Units: ms
    // Comment: End of clip in video time
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT32 GetClipEnd(void) const
    {
        return GetFieldUINT32Value(7, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set clip_end field
    // Units: ms
    // Comment: End of clip in video time
    ///////////////////////////////////////////////////////////////////////
    void SetClipEnd(FIT_UINT32 clipEnd)
    {
        SetFieldUINT32Value(7, clipEnd, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_VIDEO_CLIP_MESG_HPP)
