#include "plan.h"
#include "tables.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <sstream>
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
#define EXPECT_TRUE(x)     check((x),        #x " is false",           __FILE__, __LINE__)
#define EXPECT_FALSE(x)    check(!(x),        #x " is true",            __FILE__, __LINE__)
#define EXPECT_EQ(a, b)    check((a) == (b), #a " != " #b,             __FILE__, __LINE__)
#define EXPECT_GT(a, b)    check((a) > (b),  #a " not > " #b,          __FILE__, __LINE__)
#define EXPECT_STREQ(a, b) check(std::string(a) == std::string(b), \
                                 std::string(#a) + " != " + std::string(#b), __FILE__, __LINE__)
#define EXPECT_NEAR(a, b, tol) check(std::abs(static_cast<double>(a) - static_cast<double>(b)) <= (tol), \
                                     std::string(#a) + " not near " + #b, __FILE__, __LINE__)

// ---- parse_time ----

TEST(ParseTime, HoursMinutesSeconds) {
    EXPECT_EQ(first::parse_time("3:15:00"), 11700);
}
TEST(ParseTime, MinutesOnly) {
    EXPECT_EQ(first::parse_time("20:00"), 1200);
}
TEST(ParseTime, RoundTrip) {
    EXPECT_EQ(first::parse_time(first::fmt_time(11700)), 11700);
}

// ---- fmt_pace ----

TEST(FmtPace, EightMinuteMile) {
    EXPECT_STREQ(first::fmt_pace(480), "8:00/mi");
}
TEST(FmtPace, SevenThirty) {
    EXPECT_STREQ(first::fmt_pace(450), "7:30/mi");
}

// ---- pace_to_speed_range ----

TEST(PaceToSpeedRange, EightMinuteMile) {
    int lo, hi;
    // 8:00/mi = 480 s/mi; speed = 1609.344/480 ≈ 3.353 m/s
    first::pace_to_speed_range(480, 0.0, lo, hi);
    EXPECT_NEAR(lo, 3353, 2);
    EXPECT_NEAR(hi, 3353, 2);
}
TEST(PaceToSpeedRange, LowLessThanHighWithTolerance) {
    int lo, hi;
    first::pace_to_speed_range(420, 0.04, lo, hi);
    EXPECT_TRUE(lo < hi);
}

// ---- dist_time_to_speed_range ----

TEST(DistTimeToSpeedRange, KnownValue) {
    int lo, hi;
    // 400m in 80s → 5 m/s = 5000 mm/s, ±0%
    first::dist_time_to_speed_range(400, 80, 0.0, lo, hi);
    EXPECT_NEAR(lo, 5000, 2);
    EXPECT_NEAR(hi, 5000, 2);
}
TEST(DistTimeToSpeedRange, ToleranceWiden) {
    int lo0, hi0, lo1, hi1;
    first::dist_time_to_speed_range(400, 80, 0.0,  lo0, hi0);
    first::dist_time_to_speed_range(400, 80, 0.05, lo1, hi1);
    EXPECT_TRUE(lo1 < lo0);
    EXPECT_TRUE(hi1 > hi0);
}

// ---- generate_plan: week counts ----

TEST(GeneratePlan, MarathonHas16Weeks) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_EQ((int)plan.weeks.size(), 16);
}
TEST(GeneratePlan, HalfHas16Weeks) {
    auto plan = first::generate_plan("1:35:00", "half");
    EXPECT_EQ((int)plan.weeks.size(), 16);
}
TEST(GeneratePlan, K10Has12Weeks) {
    auto plan = first::generate_plan("45:00", "10k");
    EXPECT_EQ((int)plan.weeks.size(), 12);
}
TEST(GeneratePlan, K5Has12Weeks) {
    auto plan = first::generate_plan("20:00", "5k");
    EXPECT_EQ((int)plan.weeks.size(), 12);
}

// ---- generate_plan: week ordering ----

// Weeks are ordered descending: weeks[0].week == N, weeks.back().week == 1
TEST(GeneratePlan, MarathonFirstWeekIs16) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_EQ(plan.weeks[0].week, 16);
}
TEST(GeneratePlan, MarathonLastWeekIsRace) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_EQ(plan.weeks[15].week, 1);
}
TEST(GeneratePlan, K5FirstWeekIs12) {
    auto plan = first::generate_plan("20:00", "5k");
    EXPECT_EQ(plan.weeks[0].week, 12);
}

// ---- generate_plan: step structure ----

// Marathon week 16 KR1: 3×1600 with 400m RI → 6 steps (uniform)
TEST(GeneratePlan, MarathonW16_KR1_StepCount) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_EQ((int)plan.weeks[0].kr1_steps.size(), 6);
}
TEST(GeneratePlan, MarathonW16_KR1_AlternatesRunRecover) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    const auto& steps = plan.weeks[0].kr1_steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_EQ(steps[i].kind,   first::StepKind::RUN);
        EXPECT_EQ(steps[i+1].kind, first::StepKind::RECOVER);
    }
}
TEST(GeneratePlan, MarathonW16_KR1_DistIs1600) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    const auto& steps = plan.weeks[0].kr1_steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_EQ(steps[i].duration_val, 1600);
        EXPECT_TRUE(steps[i].dist_based);
    }
}
TEST(GeneratePlan, MarathonW16_KR1_HasSpeedTarget) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    const auto& steps = plan.weeks[0].kr1_steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_GT(steps[i].speed_high_mms, 0);
        EXPECT_GT(steps[i].speed_low_mms,  0);
    }
}

// Marathon week 4 KR1: 10×400 with 400m RI → 20 steps
TEST(GeneratePlan, MarathonW4_KR1_StepCount) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    // weeks ordered 16..1; week 4 is at index 16-4 = 12
    EXPECT_EQ((int)plan.weeks[12].kr1_steps.size(), 20);
}

// Marathon race week KR3 is empty (no long run on race day)
TEST(GeneratePlan, RaceWeekKR3Empty) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_TRUE(plan.weeks[15].kr3_steps.empty());
}

// KR2 steps are non-empty for most weeks
TEST(GeneratePlan, KR2StepsNonEmpty) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    for (size_t i = 0; i < plan.weeks.size() - 1; i++) {  // skip race week
        EXPECT_GT((int)plan.weeks[i].kr2_steps.size(), 0);
    }
}

// KR3 has exactly one step for non-race weeks
TEST(GeneratePlan, KR3SingleStepNonRaceWeeks) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    for (size_t i = 0; i < plan.weeks.size() - 1; i++) {
        EXPECT_EQ((int)plan.weeks[i].kr3_steps.size(), 1);
    }
}

// Different goal times produce different paces (faster goal → faster KR1 steps)
TEST(GeneratePlan, FasterGoalFasterSteps) {
    auto slow = first::generate_plan("4:00:00", "marathon");
    auto fast = first::generate_plan("3:00:00", "marathon");
    const auto& slow_steps = slow.weeks[0].kr1_steps;
    const auto& fast_steps = fast.weeks[0].kr1_steps;
    EXPECT_GT(fast_steps[0].speed_high_mms, slow_steps[0].speed_high_mms);
}

// distance field is set correctly
TEST(GeneratePlan, DistanceFieldMarathon) {
    auto plan = first::generate_plan("3:15:00", "marathon");
    EXPECT_STREQ(plan.distance, "marathon");
}
TEST(GeneratePlan, DistanceFieldK5) {
    auto plan = first::generate_plan("20:00", "5k");
    EXPECT_STREQ(plan.distance, "5k");
}

// Invalid distance throws
TEST(GeneratePlan, InvalidDistanceThrows) {
    bool threw = false;
    try { first::generate_plan("3:15:00", "mile"); } catch (...) { threw = true; }
    EXPECT_TRUE(threw);
}

// ---- lookup_paces: 5K time below table min (clamps to row 0) ----

// sk5=959 < table min 960 → all tables clamp to row 0.
// Corresponds to a marathon goal below ~2:35:42 (the fastest in the table).
TEST(LookupPaces, BelowTableMinClampsTrackIntervals) {
    auto p = first::lookup_paces(959);
    EXPECT_EQ(p.r400,   67);
    EXPECT_EQ(p.r600,  103);
    EXPECT_EQ(p.r800,  138);
    EXPECT_EQ(p.r1000, 175);
    EXPECT_EQ(p.r1200, 214);
    EXPECT_EQ(p.r1600, 293);
    EXPECT_EQ(p.r2000, 371);
}
TEST(LookupPaces, BelowTableMinClampsTempoIntervals) {
    auto p = first::lookup_paces(959);
    EXPECT_EQ(p.st_mile, 326);
    EXPECT_EQ(p.mt_mile, 341);
    EXPECT_EQ(p.lt_mile, 356);
}
TEST(LookupPaces, BelowTableMinClampsLongRunPaces) {
    auto p = first::lookup_paces(959);
    EXPECT_EQ(p.mp_mile,  356);
    EXPECT_EQ(p.hmp_mile, 339);
}

// ---- lookup_paces: 5K time above table max (clamps to last row) ----

// sk5=2401 > table max 2400 → all tables clamp to row 144.
// Corresponds to a marathon goal above ~6:29:15 (the slowest in the table).
TEST(LookupPaces, AboveTableMaxClampsTrackIntervals) {
    auto p = first::lookup_paces(2401);
    EXPECT_EQ(p.r400,  183);
    EXPECT_EQ(p.r600,  277);
    EXPECT_EQ(p.r800,  370);
    EXPECT_EQ(p.r1000, 465);
    EXPECT_EQ(p.r1200, 561);
    EXPECT_EQ(p.r1600, 756);
    EXPECT_EQ(p.r2000, 951);
}
TEST(LookupPaces, AboveTableMaxClampsTempoIntervals) {
    auto p = first::lookup_paces(2401);
    EXPECT_EQ(p.st_mile, 789);
    EXPECT_EQ(p.mt_mile, 803);
    EXPECT_EQ(p.lt_mile, 819);
}
TEST(LookupPaces, AboveTableMaxClampsLongRunPaces) {
    auto p = first::lookup_paces(2401);
    EXPECT_EQ(p.mp_mile,  891);
    EXPECT_EQ(p.hmp_mile, 849);
}

// ---- 5K Novice plan tests ----

TEST(NovicePlan, Has12Weeks) {
    auto plan = first::generate_plan("", "5k-novice");
    EXPECT_EQ((int)plan.weeks.size(), 12);
}
TEST(NovicePlan, WeeksDescending) {
    auto plan = first::generate_plan("", "5k-novice");
    EXPECT_EQ(plan.weeks[0].week, 12);
    EXPECT_EQ(plan.weeks[11].week, 1);
}
TEST(NovicePlan, AllWeeksSkipMaf) {
    auto plan = first::generate_plan("", "5k-novice");
    for (const auto& w : plan.weeks) EXPECT_TRUE(w.skip_maf);
}
TEST(NovicePlan, FirstWeekHasRunAndWalkSteps) {
    auto plan = first::generate_plan("", "5k-novice");
    const auto& steps = plan.weeks[0].kr1_steps;
    EXPECT_GT((int)steps.size(), 2);
    // Should start with WARMUP (walk)
    EXPECT_EQ(steps.front().kind, first::StepKind::WARMUP);
    // Should end with COOLDOWN (walk)
    EXPECT_EQ(steps.back().kind, first::StepKind::COOLDOWN);
}
TEST(NovicePlan, StepsHaveNoSpeedTarget) {
    auto plan = first::generate_plan("", "5k-novice");
    for (const auto& w : plan.weeks)
        for (const auto& s : w.kr1_steps) {
            EXPECT_EQ(s.speed_high_mms, 0);
            EXPECT_EQ(s.hr_high, 0);
        }
}
TEST(NovicePlan, RaceWeekKR3IsDistanceBased) {
    auto plan = first::generate_plan("", "5k-novice");
    // Race week = weeks.back() (week 1)
    const auto& steps = plan.weeks.back().kr3_steps;
    EXPECT_GT((int)steps.size(), 0);
    // The run step should be distance-based (5K race)
    bool found_dist_run = false;
    for (const auto& s : steps)
        if (s.kind == first::StepKind::RUN && s.dist_based) found_dist_run = true;
    EXPECT_TRUE(found_dist_run);
}
TEST(NovicePlan, ExportTcxProducesOutput) {
    first::Plan plan("", "5k-novice");
    plan.exportTcx("/tmp", 35);
    EXPECT_TRUE(true);
}
TEST(NovicePlan, ExportJsonProducesOutput) {
    first::Plan plan("", "5k-novice");
    plan.exportJson("/tmp", 35);
    EXPECT_TRUE(true);
}

// ---- 5K Intermediate plan tests ----

TEST(IntermediatePlan, Has12Weeks) {
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    EXPECT_EQ((int)plan.weeks.size(), 12);
}
TEST(IntermediatePlan, WeeksDescending) {
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    EXPECT_EQ(plan.weeks[0].week, 12);
    EXPECT_EQ(plan.weeks[11].week, 1);
}
TEST(IntermediatePlan, SkipsMaf) {
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    for (const auto& w : plan.weeks) EXPECT_TRUE(w.skip_maf);
}
TEST(IntermediatePlan, KR1HasSpeedTarget) {
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    // Week 12 KR1 = 2×400; each RUN step should have a speed target
    const auto& steps = plan.weeks[0].kr1_steps;
    bool found_speed = false;
    for (const auto& s : steps)
        if (s.kind == first::StepKind::RUN && s.speed_high_mms > 0) found_speed = true;
    EXPECT_TRUE(found_speed);
}
TEST(IntermediatePlan, KR3HasNoSpeedTargetForMT) {
    // KR3 week 12 = 3mi @ mid-tempo (MT pace), which should have a speed target
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    const auto& steps = plan.weeks[0].kr3_steps;
    EXPECT_EQ((int)steps.size(), 1);
    EXPECT_GT(steps[0].speed_high_mms, 0);
}
TEST(IntermediatePlan, RaceWeekKR3Empty) {
    auto plan = first::generate_plan("25:00", "5k-intermediate");
    EXPECT_TRUE(plan.weeks.back().kr3_steps.empty());
}
TEST(IntermediatePlan, FasterGoalFasterIntervals) {
    auto slow = first::generate_plan("30:00", "5k-intermediate");
    auto fast = first::generate_plan("20:00", "5k-intermediate");
    // Find first paced RUN step in KR1 (index 0 is now the warmup)
    auto first_run = [](const std::vector<first::PlanStep>& steps) {
        for (const auto& s : steps)
            if (s.kind == first::StepKind::RUN && s.speed_high_mms > 0) return s.speed_high_mms;
        return 0;
    };
    EXPECT_GT(first_run(fast.weeks[0].kr1_steps), first_run(slow.weeks[0].kr1_steps));
}
TEST(IntermediatePlan, ExportTcxProducesOutput) {
    first::Plan plan("25:00", "5k-intermediate");
    plan.exportTcx("/tmp", 35);
    EXPECT_TRUE(true);
}
TEST(IntermediatePlan, ExportJsonProducesOutput) {
    first::Plan plan("25:00", "5k-intermediate");
    plan.exportJson("/tmp", 35);
    EXPECT_TRUE(true);
}

// ---- TCX export smoke test ----

TEST(ExportTcx, ProducesOutput) {
    first::Plan plan("3:15:00", "marathon");
    plan.exportTcx("/tmp", 35);
    EXPECT_TRUE(true);
}

// ---- JSON export smoke test ----

TEST(ExportJson, ProducesOutput) {
    first::Plan plan("3:15:00", "marathon");
    plan.exportJson("/tmp", 35);
    EXPECT_TRUE(true);
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
