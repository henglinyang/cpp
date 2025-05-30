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


#if !defined(FIT_DEVELOPER_FIELD_DESCRIPTION_HPP)
#define FIT_DEVELOPER_FIELD_DESCRIPTION_HPP

#include "fit_field_description_mesg.hpp"
#include "fit_developer_data_id_mesg.hpp"
#include <vector>

namespace fit
{
class DeveloperFieldDescription
{
public:
    DeveloperFieldDescription() = delete;
    DeveloperFieldDescription(const DeveloperFieldDescription& other);
    DeveloperFieldDescription(const FieldDescriptionMesg& desc, const DeveloperDataIdMesg& developer);
    virtual ~DeveloperFieldDescription();

    FIT_UINT32 GetApplicationVersion() const;
    FIT_UINT8 GetFieldDefinitionNumber() const;
    std::vector<FIT_UINT8> GetApplicationId() const;

private:
    FieldDescriptionMesg* description;
    DeveloperDataIdMesg* developer;
};

} // namespace fit

#endif // defined(FIT_FIELD_DEFINITION_HPP)
