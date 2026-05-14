#pragma once
#include "tables.h"
#include <string>
#include <vector>

namespace first {

enum class Distance { K5, K10, HALF, MARATHON };

struct WeekPlan {
    int week;          // 1 = race week
    std::string kr1;   // Key Run #1: Track Repeats (full description)
    std::string kr2;   // Key Run #2: Tempo Run (full description)
    std::string kr3;   // Key Run #3: Long Run (full description)
};

struct TrainingPlan {
    std::string header;
    Paces paces;
    std::vector<WeekPlan> weeks;  // ordered week 16..1
};

// Generate a training plan.
// goal: "H:MM:SS" finish time for the chosen distance.
// distance: "5k", "10k", "half", "marathon".
TrainingPlan generate_plan(const std::string& goal, const std::string& distance);

// Print the plan to stdout.
void print_plan(const TrainingPlan& plan);

} // namespace first
