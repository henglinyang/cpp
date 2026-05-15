#include "maf.h"

#include <cassert>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// ---- minimal test harness ----

struct TestCase { std::string name; std::function<void()> fn; };
static std::vector<TestCase>& tests() {
    static std::vector<TestCase> v;
    return v;
}
struct RegisterTest {
    RegisterTest(const char* n, std::function<void()> f) { tests().push_back({n, f}); }
};
#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static RegisterTest reg_##suite##_##name(#suite "." #name, test_##suite##_##name); \
    static void test_##suite##_##name()

static void check(bool cond, const std::string& msg, const char* file, int line) {
    if (!cond) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + msg);
}
#define EXPECT_TRUE(x)  check((x),       #x " is false",  __FILE__, __LINE__)
#define EXPECT_EQ(a, b) check((a)==(b),  #a " != " #b,    __FILE__, __LINE__)

// ---- helpers ----

static constexpr uint8_t kDurTime    = 0;
static constexpr uint8_t kTgtHr      = 1;
static constexpr uint8_t kInWarmup   = 2;
static constexpr uint8_t kInCooldown = 3;

static WorkoutData make_empty() {
    WorkoutData w;
    return w;
}

// ---- push_maf_warmup ----

TEST(Warmup, AppendsFiveSteps) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    EXPECT_EQ((int)w.steps.size(), 5);
}

TEST(Warmup, AdvancesIdxByFive) {
    WorkoutData w = make_empty();
    uint16_t idx = 3;
    push_maf_warmup(w, idx, 900, 145);
    EXPECT_EQ((int)idx, 8);
}

TEST(Warmup, StepIndicesSequential) {
    WorkoutData w = make_empty();
    uint16_t idx = 2;
    push_maf_warmup(w, idx, 900, 145);
    for (int i = 0; i < 5; i++)
        EXPECT_EQ((int)w.steps[i].step_index, 2 + i);
}

TEST(Warmup, DurationEvenSplit) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    // 900s / 5 = 180s = 180000ms
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(w.steps[i].duration_value, 180000u);
}

TEST(Warmup, DurationTypeIsTime) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 600, 145);
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(w.steps[i].duration_type, kDurTime);
}

TEST(Warmup, IntensityIsWarmup) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(w.steps[i].intensity, kInWarmup);
}

TEST(Warmup, TargetTypeIsHR) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(w.steps[i].target_type, kTgtHr);
}

TEST(Warmup, BothTargetBoundsSet) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(w.steps[i].has_target_low);
        EXPECT_TRUE(w.steps[i].has_target_high);
    }
}

// maf_hr=145: delta=(145-90)/5=11 → [90,101],[101,112],[112,123],[123,134],[134,145]
TEST(Warmup, HRRangesAscendingExact) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    EXPECT_EQ(w.steps[0].target_low,  90u);
    EXPECT_EQ(w.steps[0].target_high, 101u);
    EXPECT_EQ(w.steps[1].target_low,  101u);
    EXPECT_EQ(w.steps[1].target_high, 112u);
    EXPECT_EQ(w.steps[4].target_low,  134u);
    EXPECT_EQ(w.steps[4].target_high, 145u);
}

TEST(Warmup, FirstStepLoIs90) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 140);
    EXPECT_EQ(w.steps[0].target_low, 90u);
}

TEST(Warmup, LastStepHiIsMafHr) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    // maf_hr=143: delta=10 (53/5), last hi must equal 143 not 140
    push_maf_warmup(w, idx, 900, 143);
    EXPECT_EQ(w.steps[4].target_high, 143u);
}

TEST(Warmup, StepsMonotonicallyIncreasing) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(w.steps[i].target_low  < w.steps[i+1].target_low);
        EXPECT_TRUE(w.steps[i].target_high < w.steps[i+1].target_high);
    }
}

TEST(Warmup, ConsecutiveStepsBoundariesAlign) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    // Each step's hi should equal next step's lo.
    for (int i = 0; i < 4; i++)
        EXPECT_EQ(w.steps[i].target_high, w.steps[i+1].target_low);
}

// ---- push_maf_cooldown ----

TEST(Cooldown, AppendsFiveSteps) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 145);
    EXPECT_EQ((int)w.steps.size(), 5);
}

TEST(Cooldown, IntensityIsCooldown) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 145);
    for (int i = 0; i < 5; i++)
        EXPECT_EQ(w.steps[i].intensity, kInCooldown);
}

TEST(Cooldown, HRRangesDescending) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 145);
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(w.steps[i].target_low  > w.steps[i+1].target_low);
        EXPECT_TRUE(w.steps[i].target_high > w.steps[i+1].target_high);
    }
}

TEST(Cooldown, FirstStepHiIsMafHr) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 145);
    EXPECT_EQ(w.steps[0].target_high, 145u);
}

TEST(Cooldown, LastStepLoIs90) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 145);
    EXPECT_EQ(w.steps[4].target_low, 90u);
}

TEST(Cooldown, LastStepHiIsMafHrNonExact) {
    // maf_hr=143: delta=10, last cooldown step (lowest range) hi = 90+10 = 100
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_cooldown(w, idx, 600, 143);
    EXPECT_EQ(w.steps[4].target_low,  90u);
    EXPECT_EQ(w.steps[4].target_high, 100u);
}

// ---- warmup/cooldown are exact reverses ----

TEST(Symmetry, WarmupCooldownMirror) {
    WorkoutData wu = make_empty(), cd = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(wu, idx, 900, 145);
    idx = 0;
    push_maf_cooldown(cd, idx, 900, 145);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(wu.steps[i].target_low,  cd.steps[4 - i].target_low);
        EXPECT_EQ(wu.steps[i].target_high, cd.steps[4 - i].target_high);
    }
}

// ---- sequential calls advance idx correctly ----

TEST(Sequencing, WarmupThenCooldownIdx) {
    WorkoutData w = make_empty();
    uint16_t idx = 0;
    push_maf_warmup(w, idx, 900, 145);
    push_maf_cooldown(w, idx, 600, 145);
    EXPECT_EQ((int)w.steps.size(), 10);
    EXPECT_EQ((int)idx, 10);
    // Second batch step_index starts at 5
    EXPECT_EQ((int)w.steps[5].step_index, 5);
}

// ---- main ----

int main() {
    int passed = 0, failed = 0;
    for (const auto& t : tests()) {
        try {
            t.fn();
            printf("[ OK ] %s\n", t.name.c_str());
            passed++;
        } catch (const std::exception& e) {
            printf("[FAIL] %s  —  %s\n", t.name.c_str(), e.what());
            failed++;
        }
    }
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
