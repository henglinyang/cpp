#pragma once
#include "src/fit2tcx/workout_data.h"
#include <cstdint>

// Appends 5 equal-duration HR-progressive warmup steps to wkt.
// HR range walks from 90 up to maf_hr across the total_seconds duration.
void push_maf_warmup(WorkoutData& wkt, uint16_t& idx,
                     uint32_t total_seconds, int maf_hr);

// Appends 5 equal-duration HR-progressive cooldown steps to wkt.
// HR range walks from maf_hr down to 90 across the total_seconds duration.
void push_maf_cooldown(WorkoutData& wkt, uint16_t& idx,
                       uint32_t total_seconds, int maf_hr);
