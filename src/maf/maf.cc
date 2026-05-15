#include "maf.h"

static constexpr uint8_t kDurTime    = 0;
static constexpr uint8_t kTgtHr      = 1;
static constexpr uint8_t kInWarmup   = 2;
static constexpr uint8_t kInCooldown = 3;

static constexpr int kStartHr = 90;
static constexpr int kSteps   = 5;

static void push_steps(WorkoutData& wkt, uint16_t& idx,
                       uint32_t total_seconds, uint8_t intensity,
                       int maf_hr, bool reverse) {
    const uint32_t step_ms = (total_seconds * 1000u) / kSteps;
    const int delta = (maf_hr - kStartHr) / kSteps;

    for (int i = 0; i < kSteps; i++) {
        int sub = reverse ? (kSteps - 1 - i) : i;
        int lo  = kStartHr + sub * delta;
        int hi  = (sub == kSteps - 1) ? maf_hr : (kStartHr + (sub + 1) * delta);
        WorkoutStepData s;
        s.step_index      = idx++;
        s.duration_type   = kDurTime;
        s.duration_value  = step_ms;
        s.intensity       = intensity;
        s.target_type     = kTgtHr;
        s.has_target_low  = true;
        s.target_low      = static_cast<uint32_t>(lo);
        s.has_target_high = true;
        s.target_high     = static_cast<uint32_t>(hi);
        wkt.steps.push_back(s);
    }
}

void push_maf_warmup(WorkoutData& wkt, uint16_t& idx,
                     uint32_t total_seconds, int maf_hr) {
    push_steps(wkt, idx, total_seconds, kInWarmup, maf_hr, false);
}

void push_maf_cooldown(WorkoutData& wkt, uint16_t& idx,
                       uint32_t total_seconds, int maf_hr) {
    push_steps(wkt, idx, total_seconds, kInCooldown, maf_hr, true);
}
