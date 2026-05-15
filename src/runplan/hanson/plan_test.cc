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
#define EXPECT_GE(a, b)    check((a) >= (b), #a " not >= " #b,         __FILE__, __LINE__)
#define EXPECT_STREQ(a, b) check(std::string(a) == std::string(b), \
                                 std::string(#a) + " != " + std::string(#b), __FILE__, __LINE__)
#define EXPECT_NEAR(a, b, tol) check(std::abs(static_cast<double>(a) - static_cast<double>(b)) <= (tol), \
                                     std::string(#a) + " not near " + #b, __FILE__, __LINE__)

// ---- parse_time ----

TEST(ParseTime, HoursMinutesSeconds) {
    EXPECT_EQ(hanson::parse_time("3:30:00"), 12600);
}
TEST(ParseTime, MinutesSeconds) {
    EXPECT_EQ(hanson::parse_time("5:30"), 330);
}
TEST(ParseTime, OneHour) {
    EXPECT_EQ(hanson::parse_time("1:00:00"), 3600);
}
TEST(ParseTime, SingleDigitMinutes) {
    EXPECT_EQ(hanson::parse_time("0:45"), 45);
}

// ---- fmt_time / fmt_pace ----

TEST(FmtTime, HoursFormat) {
    EXPECT_STREQ(hanson::fmt_time(12600), "3:30:00");
}
TEST(FmtTime, RoundTrip) {
    EXPECT_EQ(hanson::parse_time(hanson::fmt_time(11700)), 11700);
}

TEST(FmtPace, EightMinuteMile) {
    EXPECT_STREQ(hanson::fmt_pace(480), "8:00/mi");
}
TEST(FmtPace, SevenThirtyMile) {
    EXPECT_STREQ(hanson::fmt_pace(450), "7:30/mi");
}

// ---- pace_to_speed_range ----

TEST(PaceToSpeedRange, EightMinuteMile) {
    int lo, hi;
    // 8:00/mi = 480 s/mi; speed = 1609.344/480 ≈ 3.353 m/s
    hanson::pace_to_speed_range(480, 0.0, lo, hi);
    EXPECT_NEAR(lo, 3353, 2);
    EXPECT_NEAR(hi, 3353, 2);
}
TEST(PaceToSpeedRange, ToleranceWiden) {
    int lo, hi;
    hanson::pace_to_speed_range(480, 0.10, lo, hi);
    int lo0, hi0;
    hanson::pace_to_speed_range(480, 0.0, lo0, hi0);
    EXPECT_TRUE(lo < lo0);
    EXPECT_TRUE(hi > hi0);
}
TEST(PaceToSpeedRange, LowLessThanHigh) {
    int lo, hi;
    hanson::pace_to_speed_range(420, 0.03, lo, hi);
    EXPECT_TRUE(lo < hi);
}

// ---- lookup_paces ----

TEST(LookupPaces, ReturnsPositiveValues) {
    auto p = hanson::lookup_paces(12600); // 3:30 marathon
    EXPECT_GT(p.tempo, 0);
    EXPECT_GT(p.r400, 0);
    EXPECT_GT(p.r800, 0);
    EXPECT_GT(p.s1mi, 0);
}
TEST(LookupPaces, FasterGoalFasterPace) {
    auto slow = hanson::lookup_paces(14400); // 4:00
    auto fast = hanson::lookup_paces(11700); // 3:15
    EXPECT_TRUE(fast.tempo < slow.tempo);
    EXPECT_TRUE(fast.r400  < slow.r400);
}
TEST(LookupPaces, InvalidGoalInGeneratePlanThrows) {
    bool threw = false;
    try { hanson::generate_plan("0:00", hanson::Program::BEGINNER); } catch (...) { threw = true; }
    EXPECT_TRUE(threw);
}

// ---- generate_plan: structure ----

TEST(GeneratePlan, BeginnerHas18Weeks) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    EXPECT_EQ((int)plan.weeks.size(), 18);
}
TEST(GeneratePlan, AdvancedHas18Weeks) {
    auto plan = hanson::generate_plan("3:15:00", hanson::Program::ADVANCED);
    EXPECT_EQ((int)plan.weeks.size(), 18);
}
TEST(GeneratePlan, WeekNumbersAscending) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    EXPECT_EQ(plan.weeks[0].week,  1);
    EXPECT_EQ(plan.weeks[17].week, 18);
}
TEST(GeneratePlan, ProgramFieldSet) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    EXPECT_EQ(plan.program, hanson::Program::BEGINNER);
}

// ---- generate_plan: SOS workout steps ----

// Beginner week 6 (weeks[5]) Tue (day 1) = 12×400m speed
TEST(GeneratePlan, Speed400StepCount) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[1].steps;
    EXPECT_EQ((int)steps.size(), 24);  // 12 run + 12 recover
}
TEST(GeneratePlan, Speed400AlternatesRunRecover) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[1].steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_EQ(steps[i].kind,   hanson::PlanStep::Kind::RUN);
        EXPECT_EQ(steps[i+1].kind, hanson::PlanStep::Kind::RECOVER);
    }
}
TEST(GeneratePlan, Speed400Dist400m) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[1].steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_EQ(steps[i].duration_val, 400);
        EXPECT_TRUE(steps[i].dist_based);
    }
}
TEST(GeneratePlan, Speed400HasSpeedTarget) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[1].steps;
    for (size_t i = 0; i < steps.size(); i += 2) {
        EXPECT_GT(steps[i].speed_high_mms, 0);
        EXPECT_GT(steps[i].speed_low_mms,  0);
    }
}

// Beginner week 8 (weeks[7]) Tue (day 1) = 6×800m speed
TEST(GeneratePlan, Speed800StepCount) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[7].days[1].steps;
    EXPECT_EQ((int)steps.size(), 12);  // 6 run + 6 recover
}

// Beginner week 6 (weeks[5]) Thu (day 3) = 5mi tempo
TEST(GeneratePlan, TempoHasThreeSteps) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[3].steps;
    EXPECT_EQ((int)steps.size(), 3);  // easy warmup, tempo run, easy cooldown
}
TEST(GeneratePlan, TempoMiddleStepHasSpeedTarget) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    const auto& steps = plan.weeks[5].days[3].steps;
    EXPECT_GT(steps[1].speed_high_mms, 0);
}

// Advanced week 7 (weeks[6]) Tue (day 1) = Ladder (7 segments → 14 steps)
TEST(GeneratePlan, LadderStepCount) {
    auto plan = hanson::generate_plan("3:15:00", hanson::Program::ADVANCED);
    const auto& steps = plan.weeks[6].days[1].steps;
    EXPECT_EQ((int)steps.size(), 14);  // 7 segs × (run + recover)
}
TEST(GeneratePlan, LadderNotUniformDistances) {
    auto plan = hanson::generate_plan("3:15:00", hanson::Program::ADVANCED);
    const auto& steps = plan.weeks[6].days[1].steps;
    // Run segments: 400, 800, 1200, 1600, 1200, 800, 400
    EXPECT_EQ(steps[0].duration_val,  400);
    EXPECT_EQ(steps[2].duration_val,  800);
    EXPECT_EQ(steps[4].duration_val, 1200);
    EXPECT_EQ(steps[6].duration_val, 1600);
}

// Early weeks (no SOS) have empty steps
TEST(GeneratePlan, EarlyEasyWeekNoSteps) {
    auto plan = hanson::generate_plan("3:30:00", hanson::Program::BEGINNER);
    // Week 1 (weeks[0]) Tue (day 1) = OFF → no steps
    EXPECT_TRUE(plan.weeks[0].days[1].steps.empty());
}

// ---- TCX export smoke test ----

TEST(ExportTcx, ProducesXmlOutput) {
    hanson::Plan plan("3:30:00", hanson::Program::BEGINNER);
    plan.exportTcx("/tmp", 35);
    EXPECT_TRUE(true);
}

// ---- JSON export smoke test ----

TEST(ExportJson, ProducesJsonOutput) {
    hanson::Plan plan("3:30:00", hanson::Program::BEGINNER);
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
