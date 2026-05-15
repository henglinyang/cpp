#include "tcx_export.h"
#include "src/fit2tcx/fit2tcx.h"
#include "src/maf/maf.h"

#include <array>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// FIT constants (numeric values from fit_profile.hpp — avoids pulling in the SDK header)
static constexpr uint8_t  kDurTime      = 0;  // TIME       (ms)
static constexpr uint8_t  kDurDist      = 1;  // DISTANCE   (cm)
static constexpr uint8_t  kDurOpen      = 5;  // OPEN
static constexpr uint8_t  kTgtSpeed     = 0;  // SPEED      (mm/s)
static constexpr uint8_t  kTgtHr        = 1;  // HEART_RATE (BPM)
static constexpr uint8_t  kTgtOpen      = 2;  // OPEN
static constexpr uint8_t  kInActive     = 0;  // ACTIVE
static constexpr uint8_t  kInRest       = 1;  // REST
static constexpr uint8_t  kInWarmup     = 2;  // WARMUP
static constexpr uint8_t  kInCooldown   = 3;  // COOLDOWN
static constexpr uint8_t  kSportRunning = 1;  // RUNNING

namespace first {

static bool is_uniform_interval(const std::vector<PlanStep>& steps) {
    if (steps.size() < 4 || steps.size() % 2 != 0) return false;
    const int run0 = steps[0].duration_val;
    const int jog0 = steps[1].duration_val;
    for (size_t j = 0; j < steps.size(); j += 2) {
        if (steps[j].kind   != StepKind::RUN)     return false;
        if (steps[j+1].kind != StepKind::RECOVER) return false;
        if (steps[j].duration_val   != run0) return false;
        if (steps[j+1].duration_val != jog0) return false;
    }
    return true;
}


static WorkoutStepData plan_step_to_workout(const PlanStep& ps, uint16_t idx) {
    WorkoutStepData s;
    s.step_index = idx;

    if (ps.dist_based) {
        s.duration_type  = kDurDist;
        s.duration_value = static_cast<uint32_t>(ps.duration_val) * 100u; // m → cm
    } else {
        s.duration_type  = kDurTime;
        s.duration_value = static_cast<uint32_t>(ps.duration_val) * 1000u; // s → ms
    }

    switch (ps.kind) {
        case StepKind::WARMUP:   s.intensity = kInWarmup;   break;
        case StepKind::RUN:      s.intensity = kInActive;   break;
        case StepKind::RECOVER:  s.intensity = kInRest;     break;
        case StepKind::COOLDOWN: s.intensity = kInCooldown; break;
    }

    if (ps.hr_high > 0) {
        s.target_type = kTgtHr;
        if (ps.hr_low > 0) { s.has_target_low = true; s.target_low = ps.hr_low; }
        s.has_target_high = true;
        s.target_high = static_cast<uint32_t>(ps.hr_high);
    } else if (ps.speed_high_mms > 0) {
        s.target_type = kTgtSpeed;
        if (ps.speed_low_mms > 0) { s.has_target_low = true; s.target_low = ps.speed_low_mms; }
        s.has_target_high = true;
        s.target_high = static_cast<uint32_t>(ps.speed_high_mms);
    } else {
        s.target_type = kTgtOpen;
    }

    if (!ps.label.empty()) { s.has_name = true; s.name = ps.label; }
    return s;
}

static WorkoutData build_workout(const std::vector<PlanStep>& ps_list,
                                  const std::string& name, int maf_hr) {
    WorkoutData wkt;
    wkt.has_sport = true;
    wkt.sport     = kSportRunning;
    wkt.has_name  = true;
    wkt.name      = name;

    uint16_t idx = 0;
    push_maf_warmup(wkt, idx, 900, maf_hr);

    if (is_uniform_interval(ps_list)) {
        uint32_t reps = static_cast<uint32_t>(ps_list.size() / 2);
        WorkoutStepData repeat;
        repeat.step_index  = idx++;
        repeat.is_repeat   = true;
        repeat.repetitions = reps;
        uint16_t child_idx = idx;
        repeat.children.push_back(plan_step_to_workout(ps_list[0], child_idx));
        repeat.children.push_back(plan_step_to_workout(ps_list[1], child_idx + 1));
        idx = child_idx + 2;
        wkt.steps.push_back(repeat);
    } else {
        for (const auto& ps : ps_list)
            wkt.steps.push_back(plan_step_to_workout(ps, idx++));
    }

    push_maf_cooldown(wkt, idx, 600, maf_hr);
    return wkt;
}

static void for_each_kr(const TrainingPlan& plan, int age,
                        std::function<void(const WorkoutData&, int week, const char*)> fn) {
    const int maf_hr = 180 - age;
    struct KREntry { const char* tag; const std::vector<PlanStep>* steps; };
    for (const auto& week : plan.weeks) {
        std::array<KREntry, 3> krs = {{
            {"kr1", &week.kr1_steps},
            {"kr2", &week.kr2_steps},
            {"kr3", &week.kr3_steps},
        }};
        for (const auto& kr : krs) {
            if (kr.steps->empty()) continue;
            std::string name = "first-" + plan.distance + "-w" + std::to_string(week.week)
                             + "-" + kr.tag;
            fn(build_workout(*kr.steps, name, maf_hr), week.week, kr.tag);
        }
    }
}

void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age) {
    for_each_kr(plan, age, [&](const WorkoutData& wkt, int week, const char* tag) {
        std::string path = outdir + "/week_" + std::to_string(week) + "_" + tag + ".tcx";
        std::ofstream ofs(path);
        if (!ofs) throw std::runtime_error("cannot open: " + path);
        writeWorkoutTcx(wkt, ofs);
    });
}

void export_plan_to_json(const TrainingPlan& plan, const std::string& outdir, int age) {
    for_each_kr(plan, age, [&](const WorkoutData& wkt, int week, const char* tag) {
        std::string path = outdir + "/week_" + std::to_string(week) + "_" + tag + ".json";
        std::ofstream ofs(path);
        if (!ofs) throw std::runtime_error("cannot open: " + path);
        writeWorkoutJson(wkt, ofs);
        ofs << "\n";
    });
}

} // namespace first
