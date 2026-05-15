#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct WorkoutStepData {
    uint16_t step_index = 0;
    uint8_t duration_type = 0;
    uint32_t duration_value = 0;
    uint8_t target_type = 2; // Open
    uint32_t target_value = 0;
    // Custom range for HEART_RATE (BPM) or SPEED (mm/s) targets.
    bool has_target_low = false, has_target_high = false;
    uint32_t target_low = 0, target_high = 0;
    uint8_t intensity = 0;
    std::string name;
    bool has_name = false;

    // Repeat group (xsi:type="Repeat_t"): children defines the repeated template.
    bool is_repeat = false;
    uint32_t repetitions = 0;
    std::vector<WorkoutStepData> children;
};

struct WorkoutData {
    std::string name;
    bool has_name = false;
    uint8_t sport = 0;
    bool has_sport = false;
    std::vector<WorkoutStepData> steps;
};
