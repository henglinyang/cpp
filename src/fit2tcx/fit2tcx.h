#pragma once
#include "workout_data.h"
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
void writeWorkoutJson(const WorkoutData& workout, std::ostream& out);

// Decodes a FIT binary stream. Throws std::runtime_error on fatal decode failure.
FitData decodeFit(std::istream& file);
