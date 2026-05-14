#pragma once
#include <string>

namespace hanson {

int parse_time(const std::string& s);   // "H:MM:SS" or "M:SS" → seconds
std::string fmt_time(int sec);          // seconds → "H:MM:SS"
std::string fmt_pace(int sec_per_mile); // seconds/mile → "M:SS"

struct Paces {
    int marathon_sec;    // marathon goal
    int hmp_sec;         // half-marathon goal
    int easy_a;          // Easy Aerobic A pace (sec/mile, slowest)
    int easy_b;          // Easy Aerobic B pace
    int easy_c;          // Easy Aerobic (moderate, used for longer easy/Sat runs)
    int moderate_long;   // Moderate Aerobic / Long Run pace
    int tempo;           // Marathon pace / Tempo pace
    int strength;        // Strength workout pace (sec/mile)
    int k10;             // 10K speed pace (sec/mile)
    int k5;              // 5K speed pace (sec/mile)

    // Speed workout interval paces (seconds per interval)
    int r400;   // 400m
    int r600;   // 600m
    int r800;   // 800m
    int r1k;    // 1000m
    int r1200;  // 1200m
    int r1600;  // 1600m (ladder uses r400,r800,r1200,r1600)

    // Strength workout interval paces (seconds per interval)
    int s1mi;   // 1-mile repeat
    int s1_5mi; // 1.5-mile repeat
    int s2mi;   // 2-mile repeat
    int s3mi;   // 3-mile repeat
};

// Look up paces for a given marathon goal time (in seconds).
// Interpolates between table rows. Throws if goal is out of range.
Paces lookup_paces(int marathon_goal_sec);

} // namespace hanson
