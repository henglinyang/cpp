#pragma once
#include "tables.h"
#include <string>
#include <vector>

namespace hanson {

enum class Program { BEGINNER, ADVANCED };

enum class DayKind {
    OFF,
    EASY,
    SPEED,
    STRENGTH,
    TEMPO,
    LONG,
    RACE,
};

// Speed/strength workout variant
enum class WorkoutVariant {
    NONE,
    S400,   // 12×400
    S600,   // 8×600
    S800,   // 6×800
    S1K,    // 5×1K
    S1200,  // 4×1200
    LADDER, // 400-800-1200-1600-1200-800-400
    S1600,  // 3×1600
    STR_1MI,   // 6×1mi
    STR_15MI,  // 4×1.5mi
    STR_2MI,   // 3×2mi
    STR_3MI,   // 2×3mi
};

struct PlanStep {
    enum class Kind { RUN, RECOVER, WARMUP, COOLDOWN };
    Kind    kind;
    bool    dist_based;    // true=meters, false=seconds
    int     duration_val;  // meters or seconds
    int     speed_low_mms  = 0;
    int     speed_high_mms = 0;
    int     hr_low  = 0;
    int     hr_high = 0;
    std::string label;
};

struct DayPlan {
    DayKind         kind      = DayKind::OFF;
    WorkoutVariant  variant   = WorkoutVariant::NONE;
    double          miles     = 0.0;   // for easy/long days
    int             tempo_mi  = 0;     // miles of tempo (for TEMPO days)
    std::string     desc;
    std::vector<PlanStep> steps;
};

struct WeekPlan {
    int week;   // 1 = first week, 18 = race week
    // Mon=0, Tue=1, Wed=2, Thu=3, Fri=4, Sat=5, Sun=6
    DayPlan days[7];
    double  total_miles = 0.0;
};

struct TrainingPlan {
    std::string header;
    Program     program;
    Paces       paces;
    std::vector<WeekPlan> weeks;  // weeks[0]=week1, ..., weeks[17]=week18
};

// Helper: convert sec/mile pace to speed range in mm/s
inline void pace_to_speed_range(int sec_per_mile, double tol,
                                 int& low_mms, int& high_mms) {
    double mps = 1609.344 / sec_per_mile;
    low_mms  = (int)(mps * (1.0 - tol) * 1000);
    high_mms = (int)(mps * (1.0 + tol) * 1000);
}

// Generate the full 18-week plan.
TrainingPlan generate_plan(const std::string& goal, Program prog);

// Print plan to stdout.
void print_plan(const TrainingPlan& plan);

} // namespace hanson
