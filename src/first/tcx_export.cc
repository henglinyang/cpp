#include "tcx_export.h"
#include "src/fit2tcx/fit2tcx.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>

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

static WorkoutStepData make_maf_step(uint16_t idx, uint32_t seconds,
                                     uint8_t intensity, int maf_hr,
                                     const char* name) {
    WorkoutStepData s;
    s.step_index    = idx;
    s.duration_type  = kDurTime;
    s.duration_value = seconds * 1000u;
    s.intensity      = intensity;
    s.target_type    = kTgtHr;
    s.has_target_high = true;
    s.target_high     = static_cast<uint32_t>(maf_hr);
    s.has_name = true;
    s.name     = name;
    return s;
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

void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age) {
    const int maf_hr = 180 - age;

    struct KREntry {
        const char* tag;
        const std::vector<PlanStep>* steps;
    };

    for (const auto& week : plan.weeks) {
        std::array<KREntry, 3> krs = {{
            {"kr1", &week.kr1_steps},
            {"kr2", &week.kr2_steps},
            {"kr3", &week.kr3_steps},
        }};

        for (const auto& kr : krs) {
            if (kr.steps->empty()) continue;

            WorkoutData wkt;
            wkt.has_sport = true;
            wkt.sport     = kSportRunning;
            wkt.has_name  = true;
            wkt.name      = plan.distance + "-w" + std::to_string(week.week)
                          + "-" + kr.tag;

            uint16_t idx = 0;
            wkt.steps.push_back(make_maf_step(idx++, 900, kInWarmup, maf_hr, "MAF Warmup"));

            for (const auto& ps : *kr.steps)
                wkt.steps.push_back(plan_step_to_workout(ps, idx++));

            wkt.steps.push_back(make_maf_step(idx++, 600, kInCooldown, maf_hr, "MAF Cooldown"));

            std::string path = outdir + "/week_" + std::to_string(week.week)
                             + "_" + kr.tag + ".tcx";
            std::ofstream ofs(path);
            if (!ofs) throw std::runtime_error("cannot open: " + path);
            writeWorkoutTcx(wkt, ofs);
        }
    }
}

} // namespace first
