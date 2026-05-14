#pragma once
#include "tables.h"
#include <string>
#include <vector>

namespace first {

enum class Distance { K5, K10, HALF, MARATHON };

// ---- Structured workout step (used for TCX export) ----

enum class StepKind {
    WARMUP,    // MAF warmup (time-based, HR target)
    RUN,       // timed/distance run with speed target
    RECOVER,   // recovery between repeats (time or distance, open target)
    COOLDOWN,  // MAF cooldown (time-based, HR target)
};

struct PlanStep {
    StepKind kind;
    bool     dist_based;   // true = duration_val is meters, false = seconds
    int      duration_val;
    // Speed target (m/s × 1000 = mm/s); 0 = open
    int      speed_low_mms  = 0;
    int      speed_high_mms = 0;
    // HR target (BPM); 0 = not set
    int      hr_low  = 0;
    int      hr_high = 0;
    std::string label;
};

// ---- Week plan ----

struct WeekPlan {
    int week;          // 1 = race week
    std::string kr1;   // Key Run #1 text description
    std::string kr2;   // Key Run #2 text description
    std::string kr3;   // Key Run #3 text description
    // Structured steps for TCX export (warmup/cooldown added at export time)
    std::vector<PlanStep> kr1_steps;
    std::vector<PlanStep> kr2_steps;
    std::vector<PlanStep> kr3_steps;
};

struct TrainingPlan {
    std::string header;
    std::string distance;   // "5k","10k","half","marathon"
    Paces paces;
    std::vector<WeekPlan> weeks;  // ordered week 16..1
};

// Convert pace (sec/mile) to speed range in mm/s with given tolerance fraction.
// tolerance=0.03 → ±3% around target speed.
inline void pace_to_speed_range(int sec_per_mile, double tolerance,
                                 int& low_mms, int& high_mms) {
    double mps = 1609.344 / sec_per_mile;
    low_mms  = (int)(mps * (1.0 - tolerance) * 1000);
    high_mms = (int)(mps * (1.0 + tolerance) * 1000);
}

// Convert distance-based target (dist_m metres in time_sec seconds) to speed range.
inline void dist_time_to_speed_range(int dist_m, int time_sec, double tolerance,
                                      int& low_mms, int& high_mms) {
    double mps = (double)dist_m / time_sec;
    low_mms  = (int)(mps * (1.0 - tolerance) * 1000);
    high_mms = (int)(mps * (1.0 + tolerance) * 1000);
}

// Generate a training plan.
// goal: "H:MM:SS" finish time for the chosen distance.
// distance: "5k", "10k", "half", "marathon".
TrainingPlan generate_plan(const std::string& goal, const std::string& distance);

// Print the plan to stdout.
void print_plan(const TrainingPlan& plan);

} // namespace first
