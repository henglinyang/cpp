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


#if !defined(FIT_CAPABILITIES_MESG_HPP)
#define FIT_CAPABILITIES_MESG_HPP

#include "fit_mesg.hpp"

namespace fit
{

class CapabilitiesMesg : public Mesg
{
public:
    class FieldDefNum final
    {
    public:
       static const FIT_UINT8 Languages = 0;
       static const FIT_UINT8 Sports = 1;
       static const FIT_UINT8 WorkoutsSupported = 21;
       static const FIT_UINT8 ConnectivitySupported = 23;
       static const FIT_UINT8 Invalid = FIT_FIELD_NUM_INVALID;
    };

    CapabilitiesMesg(void) : Mesg(Profile::MESG_CAPABILITIES)
    {
    }

    CapabilitiesMesg(const Mesg &mesg) : Mesg(mesg)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of languages
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumLanguages(void) const
    {
        return GetFieldNumValues(0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of languages field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsLanguagesValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(0);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns languages field
    // Comment: Use language_bits_x types where x is index of array.
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8Z GetLanguages(FIT_UINT8 index) const
    {
        return GetFieldUINT8ZValue(0, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set languages field
    // Comment: Use language_bits_x types where x is index of array.
    ///////////////////////////////////////////////////////////////////////
    void SetLanguages(FIT_UINT8 index, FIT_UINT8Z languages)
    {
        SetFieldUINT8ZValue(0, languages, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns number of sports
    ///////////////////////////////////////////////////////////////////////
    FIT_UINT8 GetNumSports(void) const
    {
        return GetFieldNumValues(1, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of sports field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsSportsValid(FIT_UINT8 index) const
    {
        const Field* field = GetField(1);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid(index);
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns sports field
    // Comment: Use sport_bits_x types where x is index of array.
    ///////////////////////////////////////////////////////////////////////
    FIT_SPORT_BITS_0 GetSports(FIT_UINT8 index) const
    {
        return GetFieldUINT8ZValue(1, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set sports field
    // Comment: Use sport_bits_x types where x is index of array.
    ///////////////////////////////////////////////////////////////////////
    void SetSports(FIT_UINT8 index, FIT_SPORT_BITS_0 sports)
    {
        SetFieldUINT8ZValue(1, sports, index, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of workouts_supported field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsWorkoutsSupportedValid() const
    {
        const Field* field = GetField(21);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns workouts_supported field
    ///////////////////////////////////////////////////////////////////////
    FIT_WORKOUT_CAPABILITIES GetWorkoutsSupported(void) const
    {
        return GetFieldUINT32ZValue(21, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set workouts_supported field
    ///////////////////////////////////////////////////////////////////////
    void SetWorkoutsSupported(FIT_WORKOUT_CAPABILITIES workoutsSupported)
    {
        SetFieldUINT32ZValue(21, workoutsSupported, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Checks the validity of connectivity_supported field
    // Returns FIT_TRUE if field is valid
    ///////////////////////////////////////////////////////////////////////
    FIT_BOOL IsConnectivitySupportedValid() const
    {
        const Field* field = GetField(23);
        if( FIT_NULL == field )
        {
            return FIT_FALSE;
        }

        return field->IsValueValid();
    }

    ///////////////////////////////////////////////////////////////////////
    // Returns connectivity_supported field
    ///////////////////////////////////////////////////////////////////////
    FIT_CONNECTIVITY_CAPABILITIES GetConnectivitySupported(void) const
    {
        return GetFieldUINT32ZValue(23, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

    ///////////////////////////////////////////////////////////////////////
    // Set connectivity_supported field
    ///////////////////////////////////////////////////////////////////////
    void SetConnectivitySupported(FIT_CONNECTIVITY_CAPABILITIES connectivitySupported)
    {
        SetFieldUINT32ZValue(23, connectivitySupported, 0, FIT_SUBFIELD_INDEX_MAIN_FIELD);
    }

};

} // namespace fit

#endif // !defined(FIT_CAPABILITIES_MESG_HPP)
