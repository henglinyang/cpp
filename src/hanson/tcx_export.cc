#include "tcx_export.h"
#include "src/fit2tcx/fit2tcx.h"
#include "src/maf/maf.h"

#include <array>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>

// FIT constants (numeric values from fit_profile.hpp)
static constexpr uint8_t  kDurTime      = 0;
static constexpr uint8_t  kDurDist      = 1;
static constexpr uint8_t  kTgtSpeed     = 0;
static constexpr uint8_t  kTgtHr        = 1;
static constexpr uint8_t  kTgtOpen      = 2;
static constexpr uint8_t  kInActive     = 0;
static constexpr uint8_t  kInRest       = 1;
static constexpr uint8_t  kInWarmup     = 2;
static constexpr uint8_t  kInCooldown   = 3;
static constexpr uint8_t  kSportRunning = 1;

namespace hanson {


static WorkoutStepData plan_step_to_wsd(const PlanStep& ps, uint16_t idx) {
    WorkoutStepData s;
    s.step_index = idx;

    if (ps.dist_based) {
        s.duration_type  = kDurDist;
        s.duration_value = static_cast<uint32_t>(ps.duration_val) * 100u;
    } else {
        s.duration_type  = kDurTime;
        s.duration_value = static_cast<uint32_t>(ps.duration_val) * 1000u;
    }

    switch (ps.kind) {
        case PlanStep::Kind::WARMUP:   s.intensity = kInWarmup;   break;
        case PlanStep::Kind::RUN:      s.intensity = kInActive;   break;
        case PlanStep::Kind::RECOVER:  s.intensity = kInRest;     break;
        case PlanStep::Kind::COOLDOWN: s.intensity = kInCooldown; break;
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

static const char* kDayTag[] = {"mon","tue","wed","thu","fri","sat","sun"};

// True when steps form N uniform (run, recover) pairs — collapse to Repeat_t.
static bool is_uniform_interval(const std::vector<PlanStep>& steps) {
    if (steps.size() < 4 || steps.size() % 2 != 0) return false;
    const int run0 = steps[0].duration_val;
    const int jog0 = steps[1].duration_val;
    for (size_t j = 0; j < steps.size(); j += 2) {
        if (steps[j].kind   != PlanStep::Kind::RUN)     return false;
        if (steps[j+1].kind != PlanStep::Kind::RECOVER) return false;
        if (steps[j].duration_val   != run0) return false;
        if (steps[j+1].duration_val != jog0) return false;
    }
    return true;
}

static WorkoutData build_workout(const DayPlan& day, const char* name, int maf_hr) {
    WorkoutData wkt;
    wkt.has_sport = true;
    wkt.sport     = kSportRunning;
    wkt.has_name  = true;
    wkt.name      = name;

    uint16_t idx = 0;
    push_maf_warmup(wkt, idx, 900, maf_hr);

    if (is_uniform_interval(day.steps)) {
        uint32_t reps = static_cast<uint32_t>(day.steps.size() / 2);
        WorkoutStepData repeat;
        repeat.step_index  = idx++;
        repeat.is_repeat   = true;
        repeat.repetitions = reps;
        uint16_t child_idx = idx;
        repeat.children.push_back(plan_step_to_wsd(day.steps[0], child_idx));
        repeat.children.push_back(plan_step_to_wsd(day.steps[1], child_idx + 1));
        idx = child_idx + 2;
        wkt.steps.push_back(repeat);
    } else {
        for (const auto& ps : day.steps)
            wkt.steps.push_back(plan_step_to_wsd(ps, idx++));
    }

    push_maf_cooldown(wkt, idx, 600, maf_hr);
    return wkt;
}

static void for_each_sos(const TrainingPlan& plan, int age,
                         std::function<void(const WorkoutData&, int week, int day)> fn) {
    const int maf_hr = 180 - age;
    const char* prog = plan.program == Program::BEGINNER ? "beg" : "adv";
    for (const auto& week : plan.weeks) {
        for (int d = 0; d < 7; d++) {
            const DayPlan& day = week.days[d];
            if (day.kind != DayKind::SPEED &&
                day.kind != DayKind::STRENGTH &&
                day.kind != DayKind::TEMPO) continue;
            if (day.steps.empty()) continue;
            char nbuf[16];
            snprintf(nbuf, sizeof(nbuf), "%s-w%02d-%s", prog, week.week, kDayTag[d]);
            fn(build_workout(day, nbuf, maf_hr), week.week, d);
        }
    }
}

void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age) {
    for_each_sos(plan, age, [&](const WorkoutData& wkt, int week, int d) {
        char path[128];
        snprintf(path, sizeof(path), "%s/week_%02d_%s.tcx",
                 outdir.c_str(), week, kDayTag[d]);
        std::ofstream ofs(path);
        if (!ofs) throw std::runtime_error(std::string("cannot open: ") + path);
        writeWorkoutTcx(wkt, ofs);
    });
}

void export_plan_to_json(const TrainingPlan& plan, const std::string& outdir, int age) {
    for_each_sos(plan, age, [&](const WorkoutData& wkt, int week, int d) {
        char path[128];
        snprintf(path, sizeof(path), "%s/week_%02d_%s.json",
                 outdir.c_str(), week, kDayTag[d]);
        std::ofstream ofs(path);
        if (!ofs) throw std::runtime_error(std::string("cannot open: ") + path);
        writeWorkoutJson(wkt, ofs);
        ofs << "\n";
    });
}

} // namespace hanson
