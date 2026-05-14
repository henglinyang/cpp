#pragma once
#include <string>

namespace first {

// Pace data looked up from the FIRST tables for a given equivalent 5K time.
struct Paces {
    int sk5_sec;       // equivalent 5K in seconds

    // Table 5.6: interval target times (seconds per repeat)
    int r400, r600, r800, r1000, r1200, r1600, r2000;

    // Table 5.7: tempo paces (seconds per mile)
    int st_mile, mt_mile, lt_mile;

    // Table 5.8: long run base paces (seconds per mile)
    int mp_mile, hmp_mile;

    // Derived: marathon race pace (seconds per mile) from goal
    // Set by lookup_paces() callers; 0 means not set.
    int goal_mp_mile = 0;
    int goal_hmp_mile = 0;
};

// Convert a goal time string "H:MM:SS" or "M:SS" to seconds.
int parse_time(const std::string& s);

// Format seconds as "H:MM:SS" or "M:SS".
std::string fmt_time(int seconds);

// Format seconds-per-mile as "M:SS/mi".
std::string fmt_pace(int secs_per_mile);

// Given a goal finish time (seconds) and distance ("5k","10k","half","marathon"),
// return the equivalent 5K time in seconds (interpolated from Table 2.1).
int goal_to_5k(int goal_seconds, const std::string& distance);

// Look up paces for a given 5K time in seconds (interpolates between table rows).
Paces lookup_paces(int sk5_seconds);

} // namespace first
