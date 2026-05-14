#include "plan.h"
#include "tables.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace first {

// ---- Pace formatting helpers ----


static std::string rep_str(int n, int dist_m, int time_sec, const std::string& ri) {
    // e.g. "6 x 800 (90 sec RI) @ 2:57"
    char buf[128];
    snprintf(buf, sizeof(buf), "%d x %dm (%s RI) @ %s",
             n, dist_m, ri.c_str(), fmt_time(time_sec).c_str());
    return buf;
}

static std::string rep_str_v(std::vector<std::pair<int,int>> reps,
                              const std::string& ri,
                              const Paces& p) {
    // reps = [(dist_m, time_sec), ...]
    // e.g. "1200-1000-800-600-400 (200 RI) @ 4:32-3:43-2:57-2:22-1:27"
    std::string dists, times;
    for (auto& [d, t] : reps) {
        if (!dists.empty()) { dists += '-'; times += '-'; }
        dists += std::to_string(d);
        times += fmt_time(t);
    }
    return dists + " (" + ri + " RI) @ " + times;
}

// Map repeat distance (meters) to seconds from Paces.
static int rep_time(const Paces& p, int dist_m) {
    switch (dist_m) {
        case 400:  return p.r400;
        case 600:  return p.r600;
        case 800:  return p.r800;
        case 1000: return p.r1000;
        case 1200: return p.r1200;
        case 1600: return p.r1600;
        case 2000: return p.r2000;
        default:   return 0;
    }
}

// ---- Schedule encodings ----

// KR2 segment types
enum PaceCode { EASY=0, ST, MT, LT, MP, HMP };
struct Seg { float miles; PaceCode pace; };

static std::string kr2_desc(const std::vector<Seg>& segs, const Paces& p) {
    std::string s;
    for (auto& seg : segs) {
        if (seg.miles <= 0) continue;  // skip placeholder zero-distance segments
        if (!s.empty()) s += " + ";
        char buf[64];
        const char* label = "";
        int pace_secs = 0;
        switch (seg.pace) {
            case EASY: label = "easy"; break;
            case ST:   label = "ST";  pace_secs = p.st_mile; break;
            case MT:   label = "MT";  pace_secs = p.mt_mile; break;
            case LT:   label = "LT";  pace_secs = p.lt_mile; break;
            case MP:   label = "MP";  pace_secs = p.goal_mp_mile;  break;
            case HMP:  label = "HMP"; pace_secs = p.goal_hmp_mile; break;
        }
        if (pace_secs > 0) {
            // Show "N mi @ pace (LABEL)"
            if (seg.miles == (int)seg.miles)
                snprintf(buf, sizeof(buf), "%.0f mi @ %s (%s)",
                         (double)seg.miles, fmt_pace(pace_secs).c_str(), label);
            else
                snprintf(buf, sizeof(buf), "%.1f mi @ %s (%s)",
                         (double)seg.miles, fmt_pace(pace_secs).c_str(), label);
        } else {
            if (seg.miles == (int)seg.miles)
                snprintf(buf, sizeof(buf), "%.0f mi easy", (double)seg.miles);
            else
                snprintf(buf, sizeof(buf), "%.1f mi easy", (double)seg.miles);
        }
        s += buf;
    }
    return s;
}

static std::string kr3_desc(float miles, PaceCode base, int offset_secs, const Paces& p) {
    if (base == EASY) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f mi easy", (double)miles);
        return buf;
    }
    int base_secs = (base == MP) ? p.goal_mp_mile : p.goal_hmp_mile;
    int run_pace = base_secs + offset_secs;
    const char* base_label = (base == MP) ? "MP" : "HMP";
    char buf[128];
    if (miles == (int)miles) {
        if (offset_secs == 0)
            snprintf(buf, sizeof(buf), "%.0f mi @ %s (%s)",
                     (double)miles, fmt_pace(run_pace).c_str(), base_label);
        else
            snprintf(buf, sizeof(buf), "%.0f mi @ %s (%s+%d sec/mi)",
                     (double)miles, fmt_pace(run_pace).c_str(), base_label, offset_secs);
    } else {
        if (offset_secs == 0)
            snprintf(buf, sizeof(buf), "%.1f mi @ %s (%s)",
                     (double)miles, fmt_pace(run_pace).c_str(), base_label);
        else
            snprintf(buf, sizeof(buf), "%.1f mi @ %s (%s+%d sec/mi)",
                     (double)miles, fmt_pace(run_pace).c_str(), base_label, offset_secs);
    }
    return buf;
}

// ---- 5K Training Program (Table 5.1, 12 weeks) ----

static std::vector<WeekPlan> marathon_schedule(const Paces& p) {
    // Table 5.5: Marathon Training Program
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1, V kr2segs, float mi, PaceCode base, int off) {
        weeks.push_back({w, kr1, kr2_desc(kr2segs, q), kr3_desc(mi, base, off, q)});
    };

    // Week 16 → 1 (race week = 1)
    add(16, r(3,1600,"400m"),
        {{2,EASY},{2,ST},{2,EASY}}, 13, MP, 30);

    add(15, r(4,800,"2 min"),
        {{1,EASY},{5,MP},{1,EASY}}, 15, MP, 45);

    add(14, rep_str_v({{1200,q.r1200},{1000,q.r1000},{800,q.r800},{600,q.r600},{400,q.r400}}, "200m", q),
        {{1,EASY},{5,LT},{1,EASY}}, 17, MP, 45);

    add(13, r(5,1000,"400m"),
        {{1,EASY},{4,MT},{1,EASY}}, 20, MP, 60);

    add(12, r(3,1600,"400m"),
        {{2,EASY},{3,ST},{1,EASY}}, 18, MP, 45);

    add(11, "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        {{1,EASY},{5,MT},{1,EASY}}, 20, MP, 45);

    add(10, r(6,800,"90 sec"),
        {{1,EASY},{6,LT},{1,EASY}}, 13, MP, 15);

    add(9, "2x(6x400) (90 sec RI; 2:30 btwn sets) @ " + fmt_time(q.r400),
        {{2,EASY},{3,ST},{1,EASY}}, 18, MP, 30);

    add(8, "2x1600 (60 sec RI) + 2x800 (60 sec RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        {{1,EASY},{4,MT},{1,EASY}}, 20, MP, 30);

    add(7, r(4,1200,"2 min"),
        {{10,MP},{0,EASY}}, 15, MP, 20);

    add(6, rep_str_v({{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}}, "400m", q),
        {{5,MP},{0,EASY}}, 20, MP, 30);

    add(5, r(3,1600,"400m"),
        {{10,MP},{0,EASY}}, 15, MP, 15);

    add(4, r(10,400,"400m"),
        {{8,MP},{0,EASY}}, 20, MP, 15);

    add(3, r(8,800,"90 sec"),
        {{1,EASY},{5,MT},{1,EASY}}, 13, MP, 0);

    add(2, r(5,1000,"400m"),
        {{2,EASY},{3,ST},{1,EASY}}, 10, MP, 0);

    // Race week
    weeks.push_back({1,
        r(6,400,"400m"),
        "10-min warmup + 3 mi @ " + fmt_pace(p.goal_mp_mile) + " (MP) + 10-min cooldown",
        "Marathon 26.2 mi"});

    return weeks;
}

static std::vector<WeekPlan> half_schedule(const Paces& p) {
    // Table 5.3: Half-Marathon Training Program
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1, V kr2segs, float mi, PaceCode base, int off) {
        weeks.push_back({w, kr1, kr2_desc(kr2segs, q), kr3_desc(mi, base, off, q)});
    };

    add(16, r(12,400,"90 sec"),
        {{2,EASY},{3,ST},{1,EASY}}, 8, HMP, 20);

    add(15, rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{1200,q.r1200},{800,q.r800},{600,q.r600},{400,q.r400}}, "400m", q),
        {{1,EASY},{5,MT},{1,EASY}}, 9, HMP, 20);

    add(14, r(6,800,"90 sec"),
        {{2,EASY},{3,ST},{1,EASY}}, 10, EASY, 0);  // "10 miles easy"

    add(13, r(5,1000,"400m"),
        {{1,EASY},{3,ST},{1,EASY}}, 9, HMP, 20);

    add(12, r(3,1600,"60 sec"),
        {{1,EASY},{6,LT},{1,EASY}}, 11, HMP, 30);

    add(11, "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        {{1,EASY},{2,MT},{1,EASY},{2,MT},{1,EASY}}, 10, HMP, 20);

    add(10, r(6,800,"90 sec"),
        {{1,EASY},{5,MT},{1,EASY}}, 12, HMP, 30);

    add(9, "2x(6x400) (90 sec RI; 2:30 btwn sets) @ " + fmt_time(q.r400),
        {{1,EASY},{2,MT},{1,EASY},{2,MT},{1,EASY}}, 8, HMP, 20);

    add(8, "2x1600 (60 sec RI) + 2x800 (60 sec RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        {{1,EASY},{5,MT},{1,EASY}}, 13, HMP, 30);

    add(7, r(4,1200,"2 min"),
        {{1,EASY},{6,MT},{1,EASY}}, 10, HMP, 20);

    add(6, rep_str_v({{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}}, "400m", q),
        {{1,EASY},{5,MT},{1,EASY}}, 14, HMP, 30);

    add(5, r(3,1600,"400m"),
        {{6,EASY},{0,EASY}}, 10, HMP, 20);

    add(4, r(10,400,"400m"),
        {{1,EASY},{5,MT},{1,EASY}}, 15, HMP, 30);

    add(3, "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        {{1,EASY},{5,MT},{1,EASY}}, 12, HMP, 20);

    add(2, r(5,1000,"400m"),
        {{2,EASY},{3,ST},{1,EASY}}, 8, HMP, 20);

    weeks.push_back({1,
        r(6,400,"400m"),
        "3 mi easy (no extra warmup/cooldown)",
        "Half-Marathon 13.1 mi"});

    return weeks;
}

static std::vector<WeekPlan> k10_schedule(const Paces& p) {
    // Table 5.2: 10K Training Program (12 weeks)
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1, V kr2segs, float mi, PaceCode base, int off) {
        weeks.push_back({w, kr1, kr2_desc(kr2segs, q), kr3_desc(mi, base, off, q)});
    };

    add(12, r(8,400,"400m"),  {{3,ST},{0,EASY}}, 6, HMP, 0);
    add(11, r(5,800,"400m"),  {{1,EASY},{2,ST},{1,EASY},{2,ST},{1,EASY}}, 7, HMP, 0);
    add(10, r(2,1600,"400m"), {{4,MT},{0,EASY}}, 8, HMP, 0);
    // week 9 has mixed intervals
    add(9, rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}},"400m",q),
        {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 9, HMP, 0);
    add(8, r(4,1000,"400m"),  {{2,EASY},{4,ST},{0,EASY}}, 10, HMP, 0);
    add(7, rep_str_v({{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}},"400m",q),
        {{5,MT},{0,EASY}}, 8, HMP, 0);
    add(6, r(10,400,"90 sec"), {{4,ST},{0,EASY}}, 10, HMP, 0);
    add(5, r(6,800,"90 sec"),  {{1,EASY},{2,ST},{1,EASY},{2,ST},{1,EASY}}, 8, HMP, 0);
    add(4, r(4,1200,"400m"),   {{3,ST},{0,EASY}}, 10, HMP, 0);
    add(3, r(5,1000,"400m"),   {{6,MT},{0,EASY}}, 8, HMP, 0);
    add(2, r(3,1600,"400m"),   {{3,ST},{0,EASY}}, 7, HMP, 0);
    weeks.push_back({1,
        r(6,400,"60 sec"),
        "3 mi easy (no extra warmup/cooldown)",
        "10K Race 6.2 mi"});

    return weeks;
}

static std::vector<WeekPlan> k5_schedule(const Paces& p) {
    // Table 5.1: 5K Training Program (12 weeks)
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1, V kr2segs, float mi, PaceCode base, int off) {
        weeks.push_back({w, kr1, kr2_desc(kr2segs, q), kr3_desc(mi, base, off, q)});
    };

    add(12, r(8,400,"400m"),   {{2,ST},{0,EASY}}, 5, HMP, 0);
    add(11, r(5,800,"400m"),   {{3,ST},{0,EASY}}, 6, HMP, 0);
    add(10, "2x1600 (400m RI) + 1x800 (400m RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        {{1,EASY},{2,ST},{1,EASY},{2,ST},{0,EASY}}, 5, HMP, 0);
    add(9, rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}},"400m",q),
        {{4,MT},{0,EASY}}, 6, HMP, 0);
    add(8, r(4,1000,"400m"),   {{3,ST},{0,EASY}}, 7, HMP, 0);
    add(7, rep_str_v({{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}},"400m",q),
        {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 6, HMP, 0);
    add(6, r(10,400,"90 sec"), {{4,MT},{0,EASY}}, 8, HMP, 0);
    add(5, r(6,800,"90 sec"),  {{2,EASY},{3,ST},{0,EASY}}, 7, HMP, 0);
    add(4, r(4,1200,"400m"),   {{3,ST},{0,EASY}}, 7, HMP, 0);
    add(3, r(5,1000,"400m"),   {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 7, HMP, 0);
    add(2, r(3,1600,"400m"),   {{3,ST},{3,EASY}}, 6, HMP, 0);
    weeks.push_back({1,
        r(6,400,"60 sec"),
        "3 mi easy (no extra warmup/cooldown)",
        "5K Race 3.1 mi"});

    return weeks;
}

// ---- Public API ----

TrainingPlan generate_plan(const std::string& goal, const std::string& distance) {
    int goal_sec = parse_time(goal);
    if (goal_sec <= 0)
        throw std::runtime_error("invalid goal time: " + goal);

    // Convert goal to 5K equivalent
    int sk5 = goal_to_5k(goal_sec, distance);

    Paces p = lookup_paces(sk5);

    // Set goal race paces directly from goal time
    if (distance == "marathon") {
        p.goal_mp_mile  = (int)std::round((double)goal_sec / 26.2188);
        p.goal_hmp_mile = p.hmp_mile;
    } else if (distance == "half") {
        p.goal_hmp_mile = (int)std::round((double)goal_sec / 13.1094);
        p.goal_mp_mile  = p.mp_mile;
    } else {
        p.goal_mp_mile  = p.mp_mile;
        p.goal_hmp_mile = p.hmp_mile;
    }

    // Build header
    char hdr[256];
    const char* dist_label = distance == "5k" ? "5K" :
                             distance == "10k" ? "10K" :
                             distance == "half" ? "Half-Marathon" : "Marathon";
    int weeks_count = (distance == "5k" || distance == "10k") ? 12 : 16;
    snprintf(hdr, sizeof(hdr),
             "FIRST Run Less Run Faster\n%d-Week %s Training Plan\n"
             "Goal: %s  |  Equiv 5K: %s",
             weeks_count, dist_label,
             fmt_time(goal_sec).c_str(), fmt_time(sk5).c_str());

    TrainingPlan plan;
    plan.header = hdr;
    plan.paces = p;

    if (distance == "marathon") plan.weeks = marathon_schedule(p);
    else if (distance == "half") plan.weeks = half_schedule(p);
    else if (distance == "10k")  plan.weeks = k10_schedule(p);
    else                         plan.weeks = k5_schedule(p);

    return plan;
}

void print_plan(const TrainingPlan& plan) {
    const Paces& p = plan.paces;
    printf("%s\n\n", plan.header.c_str());

    printf("PACE REFERENCE\n");
    printf("  Track Repeats (Key Run #1):\n");
    printf("    400m @ %s    600m @ %s    800m @ %s\n",
           fmt_time(p.r400).c_str(), fmt_time(p.r600).c_str(), fmt_time(p.r800).c_str());
    printf("   1000m @ %s   1200m @ %s   1600m @ %s   2000m @ %s\n",
           fmt_time(p.r1000).c_str(), fmt_time(p.r1200).c_str(),
           fmt_time(p.r1600).c_str(), fmt_time(p.r2000).c_str());
    printf("  Tempo Paces (Key Run #2):\n");
    printf("    Short Tempo (ST): %s    Mid Tempo (MT): %s    Long Tempo (LT): %s\n",
           fmt_pace(p.st_mile).c_str(), fmt_pace(p.mt_mile).c_str(), fmt_pace(p.lt_mile).c_str());
    if (p.goal_mp_mile > 0)
        printf("  Long Run (Key Run #3):    MP = %s    HMP = %s\n",
               fmt_pace(p.goal_mp_mile).c_str(), fmt_pace(p.goal_hmp_mile).c_str());
    printf("\n");

    printf("SCHEDULE  (Key Run #1 = Tu, #2 = Th, #3 = Sa)\n");
    printf("%-6s  %-54s  %-44s  %s\n", "WEEK", "KEY RUN #1 (TRACK)", "KEY RUN #2 (TEMPO)", "KEY RUN #3 (LONG)");
    printf("%s\n", std::string(170, '-').c_str());

    for (const auto& w : plan.weeks) {
        if (w.week == 1) {
            printf("\n");  // blank line before race week
        }
        printf("%-6d  %-54s  %-44s  %s\n",
               w.week, w.kr1.c_str(), w.kr2.c_str(), w.kr3.c_str());
    }
    printf("\n");
}

} // namespace first
