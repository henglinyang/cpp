#include "tcx_export.h"
#include "src/fit2tcx/fit2tcx.h"

#include <array>
#include <fstream>
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

static WorkoutStepData make_maf(uint16_t idx, uint32_t seconds,
                                 uint8_t intensity, int maf_hr,
                                 const char* name) {
    WorkoutStepData s;
    s.step_index      = idx;
    s.duration_type   = kDurTime;
    s.duration_value  = seconds * 1000u;
    s.intensity       = intensity;
    s.target_type     = kTgtHr;
    s.has_target_high = true;
    s.target_high     = static_cast<uint32_t>(maf_hr);
    s.has_name        = true;
    s.name            = name;
    return s;
}

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

void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age) {
    const int maf_hr = 180 - age;
    const char* prog = plan.program == Program::BEGINNER ? "beg" : "adv";

    static const char* kDayTag[] = {"mon","tue","wed","thu","fri","sat","sun"};

    for (const auto& week : plan.weeks) {
        for (int d = 0; d < 7; d++) {
            const DayPlan& day = week.days[d];
            if (day.kind != DayKind::SPEED &&
                day.kind != DayKind::STRENGTH &&
                day.kind != DayKind::TEMPO)
                continue;
            if (day.steps.empty()) continue;

            WorkoutData wkt;
            wkt.has_sport = true;
            wkt.sport     = kSportRunning;
            wkt.has_name  = true;
            char nbuf[64];
            snprintf(nbuf, sizeof(nbuf), "%s-w%02d-%s",
                     prog, week.week, kDayTag[d]);
            wkt.name = nbuf;

            uint16_t idx = 0;
            wkt.steps.push_back(make_maf(idx++, 900, kInWarmup,   maf_hr, "MAF Warmup"));
            for (const auto& ps : day.steps)
                wkt.steps.push_back(plan_step_to_wsd(ps, idx++));
            wkt.steps.push_back(make_maf(idx++, 600, kInCooldown, maf_hr, "MAF Cooldown"));

            char nbuf2[128];
            snprintf(nbuf2, sizeof(nbuf2), "%s/week_%02d_%s.tcx",
                     outdir.c_str(), week.week, kDayTag[d]);
            std::ofstream ofs(nbuf2);
            if (!ofs) throw std::runtime_error(std::string("cannot open: ") + nbuf2);
            writeWorkoutTcx(wkt, ofs);
        }
    }
}

} // namespace hanson
