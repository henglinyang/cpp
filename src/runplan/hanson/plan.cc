#include "plan.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace hanson {

// ── schedule encoding ─────────────────────────────────────────────────────
// Each week: 7 entries (Mon..Sun) as (DayKind, WorkoutVariant, miles_or_tempo)
// miles_or_tempo: for EASY/LONG = miles; for TEMPO = tempo miles; for SOS = 0

struct RawDay { DayKind k; WorkoutVariant v; double m; };
using Week7 = std::array<RawDay, 7>;

static constexpr RawDay OFF  = {DayKind::OFF,      WorkoutVariant::NONE, 0};
static constexpr RawDay RACE = {DayKind::RACE,     WorkoutVariant::NONE, 0};

static RawDay easy(double m) { return {DayKind::EASY, WorkoutVariant::NONE, m}; }
static RawDay lng(double m)  { return {DayKind::LONG, WorkoutVariant::NONE, m}; }
static RawDay spd(WorkoutVariant v) { return {DayKind::SPEED,    v, 0}; }
static RawDay str(WorkoutVariant v) { return {DayKind::STRENGTH, v, 0}; }
static RawDay tmp(double mi) { return {DayKind::TEMPO, WorkoutVariant::NONE, mi}; }

// Beginner: 18 weeks, cols = Mon,Tue,Wed,Thu,Fri,Sat,Sun
static const Week7 kBeginner[18] = {
  // Week 1-5: base building (all easy)
  {easy(0), easy(0),   OFF, easy(3), OFF,     easy(3), easy(4)},
  {OFF,     easy(2),   OFF, easy(3), easy(3), easy(3), easy(4)},
  {OFF,     easy(4),   OFF, easy(4), easy(4), easy(4), easy(5)},
  {OFF,     easy(5),   OFF, easy(3), easy(3), easy(5), easy(5)},
  {OFF,     easy(5),   OFF, easy(4), easy(5), easy(4), easy(6)},
  // Week 6-10: speed + tempo
  {easy(4), spd(WorkoutVariant::S400),  OFF, tmp(5),  easy(4), easy(8),  lng(8)},
  {easy(4), spd(WorkoutVariant::S600),  OFF, tmp(5),  easy(4), easy(6),  lng(10)},
  {easy(6), spd(WorkoutVariant::S800),  OFF, tmp(5),  easy(5), easy(6),  lng(10)},
  {easy(5), spd(WorkoutVariant::S1K),   OFF, tmp(8),  easy(6), easy(5),  lng(15)},
  {easy(7), spd(WorkoutVariant::S1200), OFF, tmp(8),  easy(5), easy(8),  lng(10)},
  // Week 11-17: strength + tempo
  {easy(5), str(WorkoutVariant::STR_1MI),  OFF, tmp(8),  easy(5), easy(8),  lng(16)},
  {easy(5), str(WorkoutVariant::STR_15MI), OFF, tmp(9),  easy(5), easy(8),  lng(10)},
  {easy(7), str(WorkoutVariant::STR_2MI),  OFF, tmp(9),  easy(6), easy(6),  lng(16)},
  {easy(5), str(WorkoutVariant::STR_3MI),  OFF, tmp(9),  easy(5), easy(8),  lng(10)},
  {easy(7), str(WorkoutVariant::STR_2MI),  OFF, tmp(10), easy(6), easy(6),  lng(16)},
  {easy(5), str(WorkoutVariant::STR_15MI), OFF, tmp(10), easy(5), easy(8),  lng(10)},
  {easy(7), str(WorkoutVariant::STR_1MI),  OFF, tmp(10), easy(6), easy(6),  easy(8)},
  // Week 18: taper + race
  {easy(5), easy(5), OFF, easy(6), easy(5), easy(3), RACE},
};

// Advanced: 18 weeks
static const Week7 kAdvanced[18] = {
  // Week 1-2: speed only (no tempo yet)
  {easy(0), easy(0),                    OFF, easy(6),  easy(6), easy(6), easy(8)},
  {easy(6), spd(WorkoutVariant::S400),  OFF, easy(6),  easy(6), easy(6), easy(8)},
  // Week 3-10: speed + tempo
  {easy(6), spd(WorkoutVariant::S600),   OFF, tmp(6), easy(7), easy(6),  lng(10)},
  {easy(6), spd(WorkoutVariant::S800),   OFF, tmp(6), easy(6), easy(8),  easy(8)},
  {easy(6), spd(WorkoutVariant::S1K),    OFF, tmp(6), easy(7), easy(6),  lng(12)},
  {easy(6), spd(WorkoutVariant::S1200),  OFF, tmp(7), easy(6), easy(10), easy(8)},
  {easy(6), spd(WorkoutVariant::LADDER), OFF, tmp(7), easy(7), easy(8),  lng(14)},
  {easy(6), spd(WorkoutVariant::S1600),  OFF, tmp(7), easy(6), easy(10), easy(10)},
  {easy(8), spd(WorkoutVariant::S800),   OFF, tmp(8), easy(7), easy(8),  lng(15)},
  {easy(6), spd(WorkoutVariant::S1600),  OFF, tmp(8), easy(6), easy(10), easy(10)},
  // Week 11-17: strength + tempo
  {easy(8), str(WorkoutVariant::STR_1MI),  OFF, tmp(8),   easy(7), easy(8),  lng(16)},
  {easy(6), str(WorkoutVariant::STR_15MI), OFF, tmp(9),   easy(6), easy(10), easy(10)},
  {easy(8), str(WorkoutVariant::STR_2MI),  OFF, tmp(9),   easy(7), easy(8),  lng(16)},
  {easy(6), str(WorkoutVariant::STR_3MI),  OFF, tmp(9),   easy(6), easy(10), easy(10)},
  {easy(8), str(WorkoutVariant::STR_2MI),  OFF, tmp(10),  easy(7), easy(8),  lng(16)},
  {easy(6), str(WorkoutVariant::STR_15MI), OFF, tmp(10),  easy(6), easy(10), easy(10)},
  {easy(8), str(WorkoutVariant::STR_1MI),  OFF, tmp(10),  easy(7), easy(8),  easy(8)},
  // Week 18: taper
  {easy(6), easy(5), OFF, easy(6), easy(6), easy(3), RACE},
};

// ── step builders ─────────────────────────────────────────────────────────

using PS = PlanStep;

static PS run_step_dist(int dist_m, int pace_sec_per_mile,
                        double /*tol*/, const std::string& lbl) {
    PS s;
    s.kind = PS::Kind::RUN;
    s.dist_based = true;
    s.duration_val = dist_m;
    s.speed_low_mms  = (int)(1609.344 / (pace_sec_per_mile + 10) * 1000);
    s.speed_high_mms = (int)(1609.344 / (pace_sec_per_mile - 10) * 1000);
    s.label = lbl;
    return s;
}

static PS easy_step_dist(int dist_m, const std::string& lbl) {
    PS s;
    s.kind = PS::Kind::RUN;
    s.dist_based = true;
    s.duration_val = dist_m;
    s.label = lbl;
    return s;
}

static PS recover_dist(int meters, const std::string& lbl = "jog recovery") {
    PS s;
    s.kind = PS::Kind::RECOVER;
    s.dist_based = true;
    s.duration_val = meters;
    s.label = lbl;
    return s;
}

static int miles_to_m(double mi) { return (int)(mi * 1609.344 + 0.5); }

// Build interval set: n × dist_m at pace, with jog_m recovery between
static std::vector<PS> intervals(int n, int dist_m, int pace_sec, double tol,
                                  int jog_m, const std::string& name) {
    std::vector<PS> steps;
    for (int i = 0; i < n; i++) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "%s %d/%d", name.c_str(), i+1, n);
        steps.push_back(run_step_dist(dist_m, pace_sec, tol, lbl));
        steps.push_back(recover_dist(jog_m));
    }
    return steps;
}

static std::vector<PS> build_speed_steps(WorkoutVariant v, const Paces& p) {
    // Pace for speed = sec/interval ÷ (dist_m/1609.344) → sec/mile
    auto pace_from_interval = [](int interval_sec, int dist_m) -> int {
        return (int)((double)interval_sec / dist_m * 1609.344 + 0.5);
    };

    switch (v) {
        case WorkoutVariant::S400:
            return intervals(12, 400, pace_from_interval(p.r400,400), 0.04, 400, "400m");
        case WorkoutVariant::S600:
            return intervals(8, 600, pace_from_interval(p.r600,600), 0.04, 400, "600m");
        case WorkoutVariant::S800:
            return intervals(6, 800, pace_from_interval(p.r800,800), 0.04, 400, "800m");
        case WorkoutVariant::S1K:
            return intervals(5, 1000, pace_from_interval(p.r1k,1000), 0.04, 400, "1K");
        case WorkoutVariant::S1200:
            return intervals(4, 1200, pace_from_interval(p.r1200,1200), 0.04, 400, "1200m");
        case WorkoutVariant::S1600:
            return intervals(3, 1600, pace_from_interval(p.r1600,1600), 0.04, 400, "1600m");
        case WorkoutVariant::LADDER: {
            // 400-800-1200-1600-1200-800-400
            std::vector<PS> steps;
            struct Seg { int dist; int pace_sec; };
            std::vector<Seg> segs = {
                {400, pace_from_interval(p.r400,400)},
                {800, pace_from_interval(p.r800,800)},
                {1200, pace_from_interval(p.r1200,1200)},
                {1600, pace_from_interval(p.r1600,1600)},
                {1200, pace_from_interval(p.r1200,1200)},
                {800, pace_from_interval(p.r800,800)},
                {400, pace_from_interval(p.r400,400)},
            };
            for (size_t i = 0; i < segs.size(); i++) {
                char lbl[32]; snprintf(lbl, sizeof(lbl), "%dm", segs[i].dist);
                steps.push_back(run_step_dist(segs[i].dist, segs[i].pace_sec, 0.04, lbl));
                steps.push_back(recover_dist(400));
            }
            return steps;
        }
        default: return {};
    }
}

static std::vector<PS> build_strength_steps(WorkoutVariant v, const Paces& p) {
    // Strength pace from sec/interval (s1mi etc.) → sec/mile
    auto s_pace = [](int interval_sec, int dist_m) -> int {
        return (int)((double)interval_sec / dist_m * 1609.344 + 0.5);
    };
    switch (v) {
        case WorkoutVariant::STR_1MI:
            return intervals(6, miles_to_m(1.0), s_pace(p.s1mi, miles_to_m(1.0)), 0.03, 400, "1mi");
        case WorkoutVariant::STR_15MI:
            return intervals(4, miles_to_m(1.5), s_pace(p.s1_5mi, miles_to_m(1.5)), 0.03, 800, "1.5mi");
        case WorkoutVariant::STR_2MI:
            return intervals(3, miles_to_m(2.0), s_pace(p.s2mi, miles_to_m(2.0)), 0.03, 800, "2mi");
        case WorkoutVariant::STR_3MI:
            return intervals(2, miles_to_m(3.0), s_pace(p.s3mi, miles_to_m(3.0)), 0.03, miles_to_m(1.0), "3mi");
        default: return {};
    }
}

static std::vector<PS> build_tempo_steps(int tempo_miles, int tempo_pace) {
    double wu = 1.5, cd = 1.5;
    std::vector<PS> steps;
    steps.push_back(easy_step_dist(miles_to_m(wu), "warmup"));
    char lbl[32]; snprintf(lbl, sizeof(lbl), "%dmi tempo", tempo_miles);
    steps.push_back(run_step_dist(miles_to_m(tempo_miles), tempo_pace, 0.03, lbl));
    steps.push_back(easy_step_dist(miles_to_m(cd), "cooldown"));
    return steps;
}

// ── plan generation ───────────────────────────────────────────────────────

static std::string variant_name(WorkoutVariant v) {
    switch (v) {
        case WorkoutVariant::S400:    return "12×400m";
        case WorkoutVariant::S600:    return "8×600m";
        case WorkoutVariant::S800:    return "6×800m";
        case WorkoutVariant::S1K:     return "5×1K";
        case WorkoutVariant::S1200:   return "4×1200m";
        case WorkoutVariant::LADDER:  return "Ladder 400-800-1200-1600-1200-800-400";
        case WorkoutVariant::S1600:   return "3×1600m";
        case WorkoutVariant::STR_1MI: return "Strength 6×1mi";
        case WorkoutVariant::STR_15MI:return "Strength 4×1.5mi";
        case WorkoutVariant::STR_2MI: return "Strength 3×2mi";
        case WorkoutVariant::STR_3MI: return "Strength 2×3mi";
        default: return "";
    }
}

static DayPlan make_day(const RawDay& rd, const Paces& p) {
    DayPlan d;
    d.kind    = rd.k;
    d.variant = rd.v;
    d.miles   = rd.m;

    switch (rd.k) {
        case DayKind::OFF:
            d.desc = "Off";
            break;
        case DayKind::RACE:
            d.desc = "RACE";
            break;
        case DayKind::EASY: {
            if (rd.m == 0) { d.desc = "Off"; d.kind = DayKind::OFF; break; }
            char buf[64];
            snprintf(buf, sizeof(buf), "Easy %.0f mi @ %s",
                     rd.m, fmt_pace(p.easy_a).c_str());
            d.desc = buf;
            d.steps.push_back(easy_step_dist(miles_to_m(rd.m), "easy run"));
            break;
        }
        case DayKind::LONG: {
            char buf[64];
            snprintf(buf, sizeof(buf), "Long %.0f mi @ %s",
                     rd.m, fmt_pace(p.moderate_long).c_str());
            d.desc = buf;
            d.steps.push_back(
                run_step_dist(miles_to_m(rd.m), p.moderate_long, 0.04, "long run"));
            break;
        }
        case DayKind::TEMPO: {
            d.tempo_mi = (int)rd.m;
            char buf[128];
            snprintf(buf, sizeof(buf),
                     "Tempo %d mi @ %s (1.5mi W/U + C/D)",
                     d.tempo_mi, fmt_pace(p.tempo).c_str());
            d.desc = buf;
            d.steps = build_tempo_steps(d.tempo_mi, p.tempo);
            break;
        }
        case DayKind::SPEED: {
            d.desc = variant_name(rd.v) + " speed @ "
                   + fmt_pace(p.k5) + " (1-2mi W/U + C/D)";
            d.steps = build_speed_steps(rd.v, p);
            break;
        }
        case DayKind::STRENGTH: {
            d.desc = variant_name(rd.v) + " @ "
                   + fmt_pace(p.strength) + " (1.5-3mi W/U + C/D)";
            d.steps = build_strength_steps(rd.v, p);
            break;
        }
    }
    return d;
}

TrainingPlan generate_plan(const std::string& goal, Program prog) {
    int goal_sec = parse_time(goal);
    if (goal_sec <= 0)
        throw std::runtime_error("invalid goal time: " + goal);

    TrainingPlan plan;
    plan.program = prog;
    plan.paces   = lookup_paces(goal_sec);

    char hdr[256];
    snprintf(hdr, sizeof(hdr),
             "Hansons Marathon Method — %s Program\n"
             "Goal: %s (%s)\n"
             "Paces: Easy %s–%s | Long %s | Tempo %s | Strength %s | 10K %s | 5K %s",
             prog == Program::BEGINNER ? "Beginner" : "Advanced",
             fmt_time(goal_sec).c_str(),
             fmt_pace(plan.paces.tempo).c_str(),
             fmt_pace(plan.paces.easy_a).c_str(),
             fmt_pace(plan.paces.easy_b).c_str(),
             fmt_pace(plan.paces.moderate_long).c_str(),
             fmt_pace(plan.paces.tempo).c_str(),
             fmt_pace(plan.paces.strength).c_str(),
             fmt_pace(plan.paces.k10).c_str(),
             fmt_pace(plan.paces.k5).c_str());
    plan.header = hdr;

    const Week7* sched = (prog == Program::BEGINNER) ? kBeginner : kAdvanced;

    // Fix advanced schedule (has extra OFF/easy on Mon that shift the array)
    // The kAdvanced weeks 4,6,8,10 have a shifted pattern that was mis-encoded above.
    // We correct by using a compact 7-day encoding already done above.

    for (int w = 0; w < 18; w++) {
        WeekPlan wp;
        wp.week = w + 1;
        double total = 0;
        for (int d = 0; d < 7; d++) {
            wp.days[d] = make_day(sched[w][d], plan.paces);
            total += wp.days[d].miles;
        }
        wp.total_miles = total;
        plan.weeks.push_back(wp);
    }
    return plan;
}

// ── print ─────────────────────────────────────────────────────────────────

void print_plan(const TrainingPlan& plan) {
    printf("%s\n\n", plan.header.c_str());
    printf("%-6s  %-7s  %-7s  %-7s  %-7s  %-7s  %-7s  %-7s  %s\n",
           "Week", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Total");
    printf("%s\n", std::string(90, '-').c_str());

    for (const auto& w : plan.weeks) {
        printf("%-6d", w.week);
        for (int d = 0; d < 7; d++) {
            const auto& day = w.days[d];
            std::string s;
            switch (day.kind) {
                case DayKind::OFF:      s = "Off"; break;
                case DayKind::RACE:     s = "RACE"; break;
                case DayKind::EASY:     { char b[16]; snprintf(b,16,"E%.0f",day.miles); s=b; break; }
                case DayKind::LONG:     { char b[16]; snprintf(b,16,"L%.0f",day.miles); s=b; break; }
                case DayKind::TEMPO:    { char b[16]; snprintf(b,16,"T%d",day.tempo_mi); s=b; break; }
                case DayKind::SPEED:    s = "Spd"; break;
                case DayKind::STRENGTH: s = "Str"; break;
            }
            printf("  %-7s", s.c_str());
        }
        printf("  %.0f\n", w.total_miles);
    }

    printf("\n\nKey:\n");
    printf("  E<n> = Easy run <n> miles\n");
    printf("  L<n> = Long run <n> miles\n");
    printf("  T<n> = Tempo <n> miles at marathon goal pace (+ warmup/cooldown)\n");
    printf("  Spd  = Speed workout (see pace chart)\n");
    printf("  Str  = Strength workout (see pace chart)\n");

    printf("\n\nSpeed workout details:\n");
    const Paces& p = plan.paces;
    printf("  12×400m @ %s (jog 400m between)\n", fmt_pace(p.k5).c_str());
    printf("  8×600m  @ %s\n", fmt_pace(p.k5).c_str());
    printf("  6×800m  @ %s\n", fmt_pace(p.k5).c_str());
    printf("  5×1K    @ %s\n", fmt_pace(p.k5).c_str());
    printf("  4×1200m @ %s\n", fmt_pace(p.k5).c_str());
    printf("  Ladder  400-800-1200-1600-1200-800-400 (jog recovery)\n");
    printf("  3×1600m @ %s\n", fmt_pace(p.k5).c_str());

    printf("\nStrength workout details:\n");
    printf("  6×1mi   @ %s (400m jog)\n", fmt_pace(p.strength).c_str());
    printf("  4×1.5mi @ %s (800m jog)\n", fmt_pace(p.strength).c_str());
    printf("  3×2mi   @ %s (800m jog)\n", fmt_pace(p.strength).c_str());
    printf("  2×3mi   @ %s (1mi jog)\n",  fmt_pace(p.strength).c_str());

    printf("\nAll SOS workouts include 1-2mi warmup + cooldown (beginner) or 1.5-3mi (advanced).\n");
}

Plan::Plan(const std::string& goal, Program prog)
    : plan_(generate_plan(goal, prog)) {}

void Plan::print() const { print_plan(plan_); }

} // namespace hanson
