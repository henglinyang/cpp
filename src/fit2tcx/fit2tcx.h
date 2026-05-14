#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

struct RecordData {
    uint32_t timestamp = 0;
    bool has_lat = false, has_lon = false, has_altitude = false;
    bool has_distance = false, has_speed = false, has_power = false;
    bool has_hr = false, has_cadence = false;
    double lat = 0, lon = 0, altitude = 0, distance = 0, speed = 0;
    int heart_rate = 0, cadence = 0, power = 0;
};

struct LapData {
    uint32_t start_time = 0, timestamp = 0;
    bool has_elapsed = false, has_distance = false, has_calories = false;
    bool has_avg_hr = false, has_max_hr = false, has_avg_cadence = false;
    double total_elapsed_time = 0, total_distance = 0;
    int total_calories = 0, avg_heart_rate = 0, max_heart_rate = 0, avg_cadence = 0;
};

struct SessionData {
    uint32_t start_time = 0;
    uint8_t sport = 0;
    bool has_start_time = false, has_sport = false;
};

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

struct FitData {
    std::vector<RecordData> records;
    std::vector<LapData> laps;
    SessionData session;
    bool has_session = false;
    WorkoutData workout;
    bool has_workout = false;
};

std::string fitToIso8601(uint32_t ts);
double semicirclesToDeg(int32_t sc);
const char* sportToTcx(uint8_t sport);

void writeTcx(const std::vector<RecordData>& records,
              const std::vector<LapData>& laps,
              const SessionData& session,
              bool has_session,
              std::ostream& out);

void writeWorkoutTcx(const WorkoutData& workout, std::ostream& out);

// Decodes a FIT binary stream. Throws std::runtime_error on fatal decode failure.
FitData decodeFit(std::istream& file);
