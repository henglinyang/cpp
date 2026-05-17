#include "plan.h"
#include "tables.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace first {

// ---- Text description helpers ----

static std::string rep_str(int n, int dist_m, int time_sec, const std::string& ri) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%d x %dm (%s RI) @ %s",
             n, dist_m, ri.c_str(), fmt_time(time_sec).c_str());
    return buf;
}

static std::string rep_str_v(std::vector<std::pair<int,int>> reps,
                              const std::string& ri) {
    std::string dists, times;
    for (auto& [d, t] : reps) {
        if (!dists.empty()) { dists += '-'; times += '-'; }
        dists += std::to_string(d);
        times += fmt_time(t);
    }
    return dists + " (" + ri + " RI) @ " + times;
}

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

enum PaceCode { EASY=0, ST, MT, LT, MP, HMP };
struct Seg { float miles; PaceCode pace; };

static std::string kr2_desc(const std::vector<Seg>& segs, const Paces& p) {
    std::string s;
    for (auto& seg : segs) {
        if (seg.miles <= 0) continue;
        if (!s.empty()) s += " + ";
        char buf[64];
        int pace_secs = 0;
        const char* label = "easy";
        switch (seg.pace) {
            case EASY: break;
            case ST:  label = "ST";  pace_secs = p.st_mile; break;
            case MT:  label = "MT";  pace_secs = p.mt_mile; break;
            case LT:  label = "LT";  pace_secs = p.lt_mile; break;
            case MP:  label = "MP";  pace_secs = p.goal_mp_mile;  break;
            case HMP: label = "HMP"; pace_secs = p.goal_hmp_mile; break;
        }
        if (pace_secs > 0)
            snprintf(buf, sizeof(buf), "%.4g mi @ %s (%s)",
                     (double)seg.miles, fmt_pace(pace_secs).c_str(), label);
        else
            snprintf(buf, sizeof(buf), "%.4g mi easy", (double)seg.miles);
        s += buf;
    }
    return s;
}

static std::string kr3_desc(float miles, PaceCode base, int offset_secs, const Paces& p) {
    if (base == EASY) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.4g mi easy", (double)miles);
        return buf;
    }
    int base_secs = (base == MP) ? p.goal_mp_mile : p.goal_hmp_mile;
    int run_pace = base_secs + offset_secs;
    const char* base_label = (base == MP) ? "MP" : "HMP";
    char buf[128];
    if (offset_secs == 0)
        snprintf(buf, sizeof(buf), "%.4g mi @ %s (%s)",
                 (double)miles, fmt_pace(run_pace).c_str(), base_label);
    else
        snprintf(buf, sizeof(buf), "%.4g mi @ %s (%s+%d sec/mi)",
                 (double)miles, fmt_pace(run_pace).c_str(), base_label, offset_secs);
    return buf;
}

// ---- Structured step builders ----

// Build a speed-targeted run step (distance-based repeat).
static PlanStep run_step(int dist_m, int time_sec, const std::string& lbl) {
    PlanStep s;
    s.kind = StepKind::RUN;
    s.dist_based = true;
    s.duration_val = dist_m;
    dist_time_to_speed_range(dist_m, time_sec, 0.03, s.speed_low_mms, s.speed_high_mms);
    s.label = lbl;
    return s;
}

// Build a speed-targeted run step (distance-based, pace in sec/mile).
static PlanStep run_step_pace(float miles, int pace_sec_per_mile, const std::string& lbl) {
    PlanStep s;
    s.kind = StepKind::RUN;
    s.dist_based = true;
    s.duration_val = (int)(miles * 1609.344);
    pace_to_speed_range(pace_sec_per_mile, 0.04, s.speed_low_mms, s.speed_high_mms);
    s.label = lbl;
    return s;
}

// Build an easy run step (distance-based, open target).
static PlanStep easy_step(float miles, const std::string& lbl = "easy") {
    PlanStep s;
    s.kind = StepKind::RUN;
    s.dist_based = true;
    s.duration_val = (int)(miles * 1609.344);
    s.label = lbl;
    return s;
}

// Build a time-based recovery step.
static PlanStep recover_time(int seconds, const std::string& lbl = "recovery") {
    PlanStep s;
    s.kind = StepKind::RECOVER;
    s.dist_based = false;
    s.duration_val = seconds;
    s.label = lbl;
    return s;
}

// Build a distance-based recovery step.
static PlanStep recover_dist(int meters, const std::string& lbl = "recovery") {
    PlanStep s;
    s.kind = StepKind::RECOVER;
    s.dist_based = true;
    s.duration_val = meters;
    s.label = lbl;
    return s;
}

// Build steps for n identical repeats with distance-based recovery.
static std::vector<PlanStep> repeat_steps(int n, int dist_m, int time_sec,
                                           bool ri_dist, int ri_val) {
    std::vector<PlanStep> steps;
    char lbl[32];
    for (int i = 0; i < n; i++) {
        snprintf(lbl, sizeof(lbl), "%dm rep %d/%d", dist_m, i+1, n);
        steps.push_back(run_step(dist_m, time_sec, lbl));
        steps.push_back(ri_dist ? recover_dist(ri_val) : recover_time(ri_val));
    }
    return steps;
}

// Append all elements of src into dst.
static void append(std::vector<PlanStep>& dst, std::vector<PlanStep>&& src) {
    for (auto& s : src) dst.push_back(std::move(s));
}

// Build KR2 steps from Seg list.
static std::vector<PlanStep> kr2_steps(const std::vector<Seg>& segs, const Paces& p) {
    std::vector<PlanStep> steps;
    for (auto& seg : segs) {
        if (seg.miles <= 0) continue;
        int pace_secs = 0;
        const char* lbl = "easy";
        switch (seg.pace) {
            case EASY: break;
            case ST:  lbl = "ST";  pace_secs = p.st_mile; break;
            case MT:  lbl = "MT";  pace_secs = p.mt_mile; break;
            case LT:  lbl = "LT";  pace_secs = p.lt_mile; break;
            case MP:  lbl = "MP";  pace_secs = p.goal_mp_mile; break;
            case HMP: lbl = "HMP"; pace_secs = p.goal_hmp_mile; break;
        }
        if (pace_secs > 0)
            steps.push_back(run_step_pace(seg.miles, pace_secs, lbl));
        else
            steps.push_back(easy_step(seg.miles));
    }
    return steps;
}

// Build KR3 single long-run step.
static std::vector<PlanStep> kr3_steps_fn(float miles, PaceCode base,
                                           int offset_secs, const Paces& p) {
    if (base == EASY) return {easy_step(miles, "long easy")};
    int base_secs = (base == MP) ? p.goal_mp_mile : p.goal_hmp_mile;
    int pace = base_secs + offset_secs;
    const char* lbl = (base == MP) ? "long MP" : "long HMP";
    return {run_step_pace(miles, pace, lbl)};
}

// ---- 5K Novice schedule (Table 3.1) ----
// Walk/run program: no pace targets, walking warmup/cooldown embedded in steps.

static PlanStep novice_walk_warmup(int sec) {
    PlanStep s; s.kind = StepKind::WARMUP; s.dist_based = false;
    s.duration_val = sec; s.label = "walk"; return s;
}
static PlanStep novice_walk_cooldown(int sec) {
    PlanStep s; s.kind = StepKind::COOLDOWN; s.dist_based = false;
    s.duration_val = sec; s.label = "walk"; return s;
}
static PlanStep novice_run_time(int sec) {
    PlanStep s; s.kind = StepKind::RUN; s.dist_based = false;
    s.duration_val = sec; s.label = "run"; return s;
}
static PlanStep novice_run_dist(float miles) {
    PlanStep s; s.kind = StepKind::RUN; s.dist_based = true;
    s.duration_val = (int)(miles * 1609.344); s.label = "run"; return s;
}
static PlanStep novice_walk_break(int sec) {
    PlanStep s; s.kind = StepKind::RECOVER; s.dist_based = false;
    s.duration_val = sec; s.label = "walk"; return s;
}

// Build (run_sec + walk_sec) × n with surrounding 10-min walk warmup/cooldown.
static std::vector<PlanStep> novice_interval(int run_sec, int walk_sec, int n,
                                              int wu_sec = 600, int cd_sec = 600) {
    std::vector<PlanStep> s;
    s.push_back(novice_walk_warmup(wu_sec));
    for (int i = 0; i < n; i++) {
        s.push_back(novice_run_time(run_sec));
        if (i < n - 1) s.push_back(novice_walk_break(walk_sec));
    }
    s.push_back(novice_walk_cooldown(cd_sec));
    return s;
}

// Build distance run with walk warmup/cooldown.
static std::vector<PlanStep> novice_dist_run(float miles, int wu_sec = 600, int cd_sec = 600) {
    return { novice_walk_warmup(wu_sec), novice_run_dist(miles), novice_walk_cooldown(cd_sec) };
}

// Week 5 (book week 8): W:10min, R:1mi, W:5min, (R:6min, W:1min)×3, W:10min
static std::vector<PlanStep> novice_week5_interval() {
    std::vector<PlanStep> s;
    s.push_back(novice_walk_warmup(600));
    s.push_back(novice_run_dist(1.0f));
    s.push_back(novice_walk_break(300));
    for (int i = 0; i < 3; i++) {
        s.push_back(novice_run_time(360));
        if (i < 2) s.push_back(novice_walk_break(60));
    }
    s.push_back(novice_walk_cooldown(600));
    return s;
}

static std::string novice_interval_desc(int run_sec, int walk_sec, int n) {
    char buf[128];
    snprintf(buf, sizeof(buf), "walk 10min, (run %dmin, walk %dmin)×%d, walk 10min",
             run_sec/60, walk_sec/60, n);
    return buf;
}
static std::string novice_dist_desc(float miles) {
    char buf[64];
    snprintf(buf, sizeof(buf), "walk 10min, run %.4g mi, walk 10min", (double)miles);
    return buf;
}

static std::vector<WeekPlan> k5_novice_schedule() {
    std::vector<WeekPlan> weeks;

    auto add = [&](int w, std::string d1, std::vector<PlanStep> s1,
                           std::string d2, std::vector<PlanStep> s2,
                           std::string d3, std::vector<PlanStep> s3) {
        WeekPlan wp;
        wp.week = w; wp.skip_maf = true;
        wp.kr1 = d1; wp.kr1_steps = std::move(s1);
        wp.kr2 = d2; wp.kr2_steps = std::move(s2);
        wp.kr3 = d3; wp.kr3_steps = std::move(s3);
        weeks.push_back(std::move(wp));
    };

    auto iv = novice_interval_desc;
    auto dr = novice_dist_desc;

    // Book week 1 (code week 12): all 3 workouts identical
    auto w1 = novice_interval(60, 120, 4);
    add(12, iv(60,120,4), w1, iv(60,120,4), w1, iv(60,120,4), w1);

    // Book week 2 (code week 11)
    auto w2 = novice_interval(120, 120, 3);
    add(11, iv(120,120,3), w2, iv(120,120,3), w2, iv(120,120,3), w2);

    // Book week 3 (code week 10): WO1/WO2 differ from WO3
    auto w3a = novice_interval(120, 60, 4);
    auto w3b = novice_interval(180, 120, 3);
    add(10, iv(120,60,4), w3a, iv(120,60,4), w3a,
            "walk 10min, (run 3min, walk 2min)×3, walk 10min", w3b);

    // Book week 4 (code week 9): WO3 has 5 reps
    auto w4a = novice_interval(180, 60, 4);
    auto w4b = novice_interval(180, 60, 5);
    add(9, iv(180,60,4), w4a, iv(180,60,4), w4a,
           "walk 10min, (run 3min, walk 1min)×5, walk 10min", w4b);

    // Book week 5 (code week 8): WO3 has 5 reps of (4min run, 1min walk)
    auto w5a = novice_interval(240, 120, 4);
    auto w5b = novice_interval(240, 60, 5);
    add(8, iv(240,120,4), w5a, iv(240,120,4), w5a,
           "walk 10min, (run 4min, walk 1min)×5, walk 10min", w5b);

    // Book week 6 (code week 7): 6 reps; WO3 is (5min run, 1min walk)×5
    auto w6a = novice_interval(240, 60, 6);
    auto w6b = novice_interval(300, 60, 5);
    add(7, iv(240,60,6), w6a, iv(240,60,6), w6a,
           "walk 10min, (run 5min, walk 1min)×5, walk 10min", w6b);

    // Book week 7 (code week 6): 6 reps (5min); WO3 is (6min, 1min)×5
    auto w7a = novice_interval(300, 60, 6);
    auto w7b = novice_interval(360, 60, 5);
    add(6, iv(300,60,6), w7a, iv(300,60,6), w7a,
           "walk 10min, (run 6min, walk 1min)×5, walk 10min", w7b);

    // Book week 8 (code week 5): 1mi + intervals; WO3 is just 1mi
    auto w8ab = novice_week5_interval();
    auto w8c = std::vector<PlanStep>{
        novice_walk_warmup(600), novice_run_dist(1.0f),
        novice_walk_break(300),  novice_run_dist(1.0f),
        novice_walk_cooldown(600)};
    add(5, "walk 10min, run 1mi, walk 5min, (run 6min, walk 1min)×3, walk 10min", w8ab,
           "walk 10min, run 1mi, walk 5min, (run 6min, walk 1min)×3, walk 10min", w8ab,
           "walk 10min, run 1mi, walk 5min, run 1mi, walk 10min", w8c);

    // Book week 9 (code week 4)
    add(4, dr(1.5f), novice_dist_run(1.5f, 600, 600),
           "walk 10min, run 1.5mi, walk 5min", {novice_walk_warmup(600), novice_run_dist(1.5f), novice_walk_cooldown(300)},
           dr(2.0f), {novice_walk_warmup(600), novice_run_dist(2.0f), novice_walk_cooldown(300)});

    // Book week 10 (code week 3)
    auto w10ab = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(2.0f), novice_walk_cooldown(300)};
    auto w10c  = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(2.5f), novice_walk_cooldown(300)};
    add(3, dr(2.0f), w10ab, dr(2.0f), w10ab, dr(2.5f), w10c);

    // Book week 11 (code week 2)
    auto w11ab = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(2.0f), novice_walk_cooldown(600)};
    auto w11c  = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(3.0f), novice_walk_cooldown(300)};
    add(2, dr(2.0f), w11ab, dr(2.0f), w11ab, dr(3.0f), w11c);

    // Book week 12 = race week (code week 1)
    auto w12ab = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(2.0f), novice_walk_cooldown(600)};
    auto w12c  = std::vector<PlanStep>{novice_walk_warmup(600), novice_run_dist(3.1f), novice_walk_cooldown(300)};
    add(1, dr(2.0f), w12ab, dr(2.0f), w12ab,
           "walk 10min, 5K Race 3.1mi, walk 5min", w12c);

    return weeks;
}

// ---- 5K Intermediate schedule (Table 3.2) ----
// Uses FIRST key run structure (track repeats / tempo / long), paces from 5K goal time.

static std::vector<WeekPlan> k5_intermediate_schedule(const Paces& p) {
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;

    // Wrap KR1 repeats with 10-min easy run warmup/cooldown (Table 3.2 specifies these).
    auto with_wu_cd = [](std::vector<PlanStep> steps) {
        PlanStep wu; wu.kind = StepKind::WARMUP;   wu.dist_based = false;
        wu.duration_val = 600; wu.label = "warmup run";
        PlanStep cd; cd.kind = StepKind::COOLDOWN; cd.dist_based = false;
        cd.duration_val = 600; cd.label = "cooldown run";
        steps.insert(steps.begin(), wu);
        steps.push_back(cd);
        return steps;
    };

    auto add = [&](int w, std::string kr1_txt, std::vector<PlanStep> kr1_s,
                   V kr2segs, float mi, PaceCode base, int off) {
        WeekPlan wp;
        wp.week      = w;
        wp.skip_maf  = true;
        wp.kr1       = kr1_txt;
        wp.kr2       = kr2_desc(kr2segs, q);
        wp.kr3       = kr3_desc(mi, base, off, q);
        wp.kr1_steps = with_wu_cd(std::move(kr1_s));
        wp.kr2_steps = kr2_steps(kr2segs, q);
        wp.kr3_steps = kr3_steps_fn(mi, base, off, q);
        weeks.push_back(std::move(wp));
    };

    auto ri400 = [&](int n, int d) { return repeat_steps(n, d, rep_time(q, d), true, 400); };

    // Mixed interval helpers (ascending ladder with 400m RI between each)
    auto mixed_steps = [&](std::vector<int> dists) {
        std::vector<PlanStep> s;
        for (size_t i = 0; i < dists.size(); i++) {
            int d = dists[i]; char lbl[32]; snprintf(lbl, sizeof(lbl), "%dm", d);
            int t = rep_time(q, d);
            if (t == 0) t = (d <= 200) ? q.r400/2 : 0;  // estimate 200m
            s.push_back(run_step(d, t > 0 ? t : 1, lbl));
            if (i < dists.size() - 1) s.push_back(recover_dist(400));
        }
        return s;
    };
    auto mixed_desc = [&](std::vector<int> dists) {
        std::string txt;
        for (size_t i = 0; i < dists.size(); i++) {
            if (i) txt += '-';
            txt += std::to_string(dists[i]);
        }
        txt += " (400m RI)";
        return txt;
    };

    // book week 1 (code week 12)
    add(12, r(2,400,"400m"), ri400(2,400), {{1,EASY},{1,ST},{1,EASY}}, 3, MT, 0);
    // book week 2 (code week 11)
    add(11, r(3,400,"400m"), ri400(3,400), {{1,EASY},{1,ST},{1,EASY}}, 3, MT, 0);
    // book week 3 (code week 10)
    add(10, r(4,400,"400m"), ri400(4,400), {{1,EASY},{1,ST},{1,EASY}}, 3, MT, 0);
    // book week 4 (code week 9): 2×400 + 1×800
    add(9, mixed_desc({400,400,800}), mixed_steps({400,400,800}),
        {{1,EASY},{1.5f,ST},{1,EASY}}, 3.5f, MT, 0);
    // book week 5 (code week 8): 400-600-800
    add(8, mixed_desc({400,600,800}), mixed_steps({400,600,800}),
        {{1,EASY},{1.5f,ST},{1,EASY}}, 4, MT, 0);
    // book week 6 (code week 7)
    add(7, r(5,400,"400m"), ri400(5,400), {{1,EASY},{1.5f,ST},{1,EASY}}, 4, MT, 0);
    // book week 7 (code week 6): 400 + 2×800
    add(6, mixed_desc({400,800,800}), mixed_steps({400,800,800}),
        {{1,EASY},{1.5f,ST},{1,EASY}}, 4.5f, MT, 0);
    // book week 8 (code week 5): 2×1000
    add(5, r(2,1000,"400m"), ri400(2,1000), {{1,EASY},{2,ST},{1,EASY}}, 4.5f, MT, 0);
    // book week 9 (code week 4)
    add(4, r(6,400,"400m"), ri400(6,400), {{1,EASY},{2,ST},{1,EASY}}, 5, LT, 0);
    // book week 10 (code week 3)
    add(3, r(3,800,"400m"), ri400(3,800), {{1,EASY},{2,ST},{1,EASY}}, 5, LT, 0);
    // book week 11 (code week 2): 200-400-600-800
    add(2, mixed_desc({200,400,600,800}), mixed_steps({200,400,600,800}),
        {{1,EASY},{2,ST},{1,EASY}}, 5, LT, 0);

    // Race week (code week 1)
    WeekPlan race;
    race.week     = 1;
    race.skip_maf = true;
    race.kr1      = r(4,400,"400m"); race.kr1_steps = with_wu_cd(ri400(4,400));
    race.kr2      = "2 mi easy + 10 min walk"; race.kr2_steps = {easy_step(2.0f)};
    race.kr3      = "5K Race 3.1 mi"; race.kr3_steps = {};
    weeks.push_back(std::move(race));

    return weeks;
}

// ---- Marathon schedule (Table 5.5) ----

static std::vector<WeekPlan> marathon_schedule(const Paces& p) {
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;

    auto add = [&](int w, std::string kr1_txt, std::vector<PlanStep> kr1_s,
                   V kr2segs,
                   float mi, PaceCode base, int off) {
        WeekPlan wp;
        wp.week     = w;
        wp.kr1      = kr1_txt;
        wp.kr2      = kr2_desc(kr2segs, q);
        wp.kr3      = kr3_desc(mi, base, off, q);
        wp.kr1_steps = kr1_s;
        wp.kr2_steps = kr2_steps(kr2segs, q);
        wp.kr3_steps = kr3_steps_fn(mi, base, off, q);
        weeks.push_back(std::move(wp));
    };

    // KR1 step builder helpers
    auto ri400 = [&](int n, int d) {
        return repeat_steps(n, d, rep_time(q,d), true, 400);
    };
    auto ri2min = [&](int n, int d) {
        return repeat_steps(n, d, rep_time(q,d), false, 120);
    };
    auto ri90s = [&](int n, int d) {
        return repeat_steps(n, d, rep_time(q,d), false, 90);
    };

    // Descending ladder 1200-1000-800-600-400 (200m RI)
    auto ladder_w14 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{1200,q.r1200},{1000,q.r1000},{800,q.r800},{600,q.r600},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i];
            char lbl[32]; snprintf(lbl, sizeof(lbl), "%dm", d);
            s.push_back(run_step(d, t, lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(200));
        }
        return s;
    };

    // 2x1200 + 4x800 combined
    auto combined_w11 = [&]() {
        std::vector<PlanStep> s;
        for (int i = 0; i < 2; i++) {
            s.push_back(run_step(1200, q.r1200, "1200m"));
            s.push_back(recover_time(120));
        }
        for (int i = 0; i < 4; i++) {
            s.push_back(run_step(800, q.r800, "800m"));
            if (i < 3) s.push_back(recover_time(120));
        }
        return s;
    };

    // 2x(6x400) with 90s RI within set, 2:30 between sets
    auto double_6x400 = [&]() {
        std::vector<PlanStep> s;
        for (int set = 0; set < 2; set++) {
            for (int i = 0; i < 6; i++) {
                char lbl[32]; snprintf(lbl, sizeof(lbl), "400m set%d rep%d", set+1, i+1);
                s.push_back(run_step(400, q.r400, lbl));
                if (set < 1 || i < 5) s.push_back(recover_time(90));
            }
            if (set == 0) s.push_back(recover_time(150));
        }
        return s;
    };

    // 2x1600 + 2x800 (60s RI)
    auto combined_w8 = [&]() {
        std::vector<PlanStep> s;
        for (int i = 0; i < 2; i++) {
            s.push_back(run_step(1600, q.r1600, "1600m"));
            s.push_back(recover_time(60));
        }
        for (int i = 0; i < 2; i++) {
            s.push_back(run_step(800, q.r800, "800m"));
            if (i < 1) s.push_back(recover_time(60));
        }
        return s;
    };

    // 1000-2000-1000-1000 (400m RI)
    auto mixed_w6 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i];
            char lbl[32]; snprintf(lbl, sizeof(lbl), "%dm", d);
            s.push_back(run_step(d, t, lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };

    // KR2 for weeks with bare MP miles (no warmup/cooldown in segs)
    auto mp_only = [&](float mi) -> V { return {{mi, MP}}; };

    add(16, r(3,1600,"400m"), ri400(3,1600),
        {{2,EASY},{2,ST},{2,EASY}}, 13, MP, 30);

    add(15, r(4,800,"2 min"), ri2min(4,800),
        {{1,EASY},{5,MP},{1,EASY}}, 15, MP, 45);

    add(14,
        rep_str_v({{1200,q.r1200},{1000,q.r1000},{800,q.r800},{600,q.r600},{400,q.r400}},"200m"),
        ladder_w14(),
        {{1,EASY},{5,LT},{1,EASY}}, 17, MP, 45);

    add(13, r(5,1000,"400m"), ri400(5,1000),
        {{1,EASY},{4,MT},{1,EASY}}, 20, MP, 60);

    add(12, r(3,1600,"400m"), ri400(3,1600),
        {{2,EASY},{3,ST},{1,EASY}}, 18, MP, 45);

    add(11,
        "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        combined_w11(),
        {{1,EASY},{5,MT},{1,EASY}}, 20, MP, 45);

    add(10, r(6,800,"90 sec"), ri90s(6,800),
        {{1,EASY},{6,LT},{1,EASY}}, 13, MP, 15);

    add(9,
        "2x(6x400) (90 sec RI; 2:30 btwn sets) @ " + fmt_time(q.r400),
        double_6x400(),
        {{2,EASY},{3,ST},{1,EASY}}, 18, MP, 30);

    add(8,
        "2x1600 (60 sec RI) + 2x800 (60 sec RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        combined_w8(),
        {{1,EASY},{4,MT},{1,EASY}}, 20, MP, 30);

    add(7, r(4,1200,"2 min"), ri2min(4,1200),
        mp_only(10), 15, MP, 20);

    add(6, rep_str_v({{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}},"400m"),
        mixed_w6(),
        mp_only(5), 20, MP, 30);

    add(5, r(3,1600,"400m"), ri400(3,1600),
        mp_only(10), 15, MP, 15);

    add(4, r(10,400,"400m"), ri400(10,400),
        mp_only(8), 20, MP, 15);

    add(3, r(8,800,"90 sec"), ri90s(8,800),
        {{1,EASY},{5,MT},{1,EASY}}, 13, MP, 0);

    add(2, r(5,1000,"400m"), ri400(5,1000),
        {{2,EASY},{3,ST},{1,EASY}}, 10, MP, 0);

    // Race week
    WeekPlan race;
    race.week = 1;
    race.kr1  = r(6,400,"400m");
    race.kr2  = "10-min warmup + 3 mi @ " + fmt_pace(p.goal_mp_mile) + " (MP) + 10-min cooldown";
    race.kr3  = "Marathon 26.2 mi";
    race.kr1_steps = ri400(6, 400);
    race.kr2_steps = {easy_step(1.0,"warmup"), run_step_pace(3, p.goal_mp_mile,"MP"), easy_step(1.0,"cooldown")};
    race.kr3_steps = {};
    weeks.push_back(std::move(race));

    return weeks;
}

// ---- Half-marathon schedule (Table 5.3) ----

static std::vector<WeekPlan> half_schedule(const Paces& p) {
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1_txt, std::vector<PlanStep> kr1_s,
                   V kr2segs, float mi, PaceCode base, int off) {
        WeekPlan wp;
        wp.week      = w;
        wp.kr1       = kr1_txt;
        wp.kr2       = kr2_desc(kr2segs, q);
        wp.kr3       = kr3_desc(mi, base, off, q);
        wp.kr1_steps = kr1_s;
        wp.kr2_steps = kr2_steps(kr2segs, q);
        wp.kr3_steps = kr3_steps_fn(mi, base, off, q);
        weeks.push_back(std::move(wp));
    };

    auto ri400 = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),true,400); };
    auto ri90s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,90); };
    auto ri2min= [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,120); };
    auto ri60s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,60); };

    auto ladder_w15 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{400,q.r400},{600,q.r600},{800,q.r800},{1200,q.r1200},{800,q.r800},{600,q.r600},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i];
            char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };
    auto combined_2x = [&](int n1, int d1, int n2, int d2, int ri_s) {
        std::vector<PlanStep> s;
        for (int i = 0; i < n1; i++) { s.push_back(run_step(d1,rep_time(q,d1),"1200m")); s.push_back(recover_time(ri_s)); }
        for (int i = 0; i < n2; i++) { s.push_back(run_step(d2,rep_time(q,d2),"800m"));  if (i < n2-1) s.push_back(recover_time(ri_s)); }
        return s;
    };
    auto double_6x400_h = [&]() {
        std::vector<PlanStep> s;
        for (int set = 0; set < 2; set++) {
            for (int i = 0; i < 6; i++) {
                char lbl[32]; snprintf(lbl,sizeof(lbl),"400m s%dr%d",set+1,i+1);
                s.push_back(run_step(400,q.r400,lbl));
                if (set < 1 || i < 5) s.push_back(recover_time(90));
            }
            if (set == 0) s.push_back(recover_time(150));
        }
        return s;
    };
    auto two_1600_2_800_60 = [&]() {
        std::vector<PlanStep> s;
        for (int i = 0; i < 2; i++) { s.push_back(run_step(1600,q.r1600,"1600m")); s.push_back(recover_time(60)); }
        for (int i = 0; i < 2; i++) { s.push_back(run_step(800,q.r800,"800m")); if (i<1) s.push_back(recover_time(60)); }
        return s;
    };
    auto mixed_1k_2k = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i]; char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };

    add(16, r(12,400,"90 sec"), ri90s(12,400),
        {{2,EASY},{3,ST},{1,EASY}}, 8, HMP, 20);
    add(15,
        rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{1200,q.r1200},{800,q.r800},{600,q.r600},{400,q.r400}},"400m"),
        ladder_w15(),
        {{1,EASY},{5,MT},{1,EASY}}, 9, HMP, 20);
    add(14, r(6,800,"90 sec"), ri90s(6,800),
        {{2,EASY},{3,ST},{1,EASY}}, 10, EASY, 0);
    add(13, r(5,1000,"400m"), ri400(5,1000),
        {{1,EASY},{3,ST},{1,EASY}}, 9, HMP, 20);
    add(12, r(3,1600,"60 sec"), ri60s(3,1600),
        {{1,EASY},{6,LT},{1,EASY}}, 11, HMP, 30);
    add(11,
        "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        combined_2x(2,1200,4,800,120),
        {{1,EASY},{2,MT},{1,EASY},{2,MT},{1,EASY}}, 10, HMP, 20);
    add(10, r(6,800,"90 sec"), ri90s(6,800),
        {{1,EASY},{5,MT},{1,EASY}}, 12, HMP, 30);
    add(9,
        "2x(6x400) (90 sec RI; 2:30 btwn sets) @ " + fmt_time(q.r400),
        double_6x400_h(),
        {{1,EASY},{2,MT},{1,EASY},{2,MT},{1,EASY}}, 8, HMP, 20);
    add(8,
        "2x1600 (60 sec RI) + 2x800 (60 sec RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        two_1600_2_800_60(),
        {{1,EASY},{5,MT},{1,EASY}}, 13, HMP, 30);
    add(7, r(4,1200,"2 min"), ri2min(4,1200),
        {{1,EASY},{6,MT},{1,EASY}}, 10, HMP, 20);
    add(6,
        rep_str_v({{1000,q.r1000},{2000,q.r2000},{1000,q.r1000},{1000,q.r1000}},"400m"),
        mixed_1k_2k(),
        {{1,EASY},{5,MT},{1,EASY}}, 14, HMP, 30);
    add(5, r(3,1600,"400m"), ri400(3,1600),
        {{6,EASY}}, 10, HMP, 20);
    add(4, r(10,400,"400m"), ri400(10,400),
        {{1,EASY},{5,MT},{1,EASY}}, 15, HMP, 30);
    add(3,
        "2x1200 (2 min RI) + 4x800 (2 min RI) @ " + fmt_time(q.r1200) + "/" + fmt_time(q.r800),
        combined_2x(2,1200,4,800,120),
        {{1,EASY},{5,MT},{1,EASY}}, 12, HMP, 20);
    add(2, r(5,1000,"400m"), ri400(5,1000),
        {{2,EASY},{3,ST},{1,EASY}}, 8, HMP, 20);

    WeekPlan race;
    race.week = 1;
    race.kr1 = r(6,400,"400m");
    race.kr2 = "3 mi easy (no extra warmup/cooldown)";
    race.kr3 = "Half-Marathon 13.1 mi";
    race.kr1_steps = ri400(6,400);
    race.kr2_steps = {easy_step(3.0)};
    race.kr3_steps = {};
    weeks.push_back(std::move(race));
    return weeks;
}

// ---- 10K schedule (Table 5.2) ----

static std::vector<WeekPlan> k10_schedule(const Paces& p) {
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1_txt, std::vector<PlanStep> kr1_s,
                   V kr2segs, float mi, PaceCode base, int off) {
        WeekPlan wp;
        wp.week      = w;
        wp.kr1       = kr1_txt;
        wp.kr2       = kr2_desc(kr2segs, q);
        wp.kr3       = kr3_desc(mi, base, off, q);
        wp.kr1_steps = kr1_s;
        wp.kr2_steps = kr2_steps(kr2segs, q);
        wp.kr3_steps = kr3_steps_fn(mi, base, off, q);
        weeks.push_back(std::move(wp));
    };

    auto ri400 = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),true,400); };
    auto ri90s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,90); };
    auto ri60s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,60); };

    auto mixed_w9 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i]; char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };
    auto desc_w7 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i]; char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };

    add(12, r(8,400,"400m"), ri400(8,400),   {{3,ST}}, 6, HMP, 0);
    add(11, r(5,800,"400m"), ri400(5,800),   {{1,EASY},{2,ST},{1,EASY},{2,ST},{1,EASY}}, 7, HMP, 0);
    add(10, r(2,1600,"400m"), ri400(2,1600), {{4,MT}}, 8, HMP, 0);
    add(9, rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}},"400m"),
        mixed_w9(), {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 9, HMP, 0);
    add(8, r(4,1000,"400m"), ri400(4,1000),  {{2,EASY},{4,ST}}, 10, HMP, 0);
    add(7, rep_str_v({{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}},"400m"),
        desc_w7(), {{5,MT}}, 8, HMP, 0);
    add(6, r(10,400,"90 sec"), ri90s(10,400), {{4,ST}}, 10, HMP, 0);
    add(5, r(6,800,"90 sec"), ri90s(6,800),   {{1,EASY},{2,ST},{1,EASY},{2,ST},{1,EASY}}, 8, HMP, 0);
    add(4, r(4,1200,"400m"), ri400(4,1200),   {{3,ST}}, 10, HMP, 0);
    add(3, r(5,1000,"400m"), ri400(5,1000),   {{6,MT}}, 8, HMP, 0);
    add(2, r(3,1600,"400m"), ri400(3,1600),   {{3,ST}}, 7, HMP, 0);

    WeekPlan race;
    race.week = 1;
    race.kr1 = r(6,400,"60 sec"); race.kr1_steps = ri60s(6,400);
    race.kr2 = "3 mi easy (no extra warmup/cooldown)";
    race.kr2_steps = {easy_step(3.0)};
    race.kr3 = "10K Race 6.2 mi"; race.kr3_steps = {};
    weeks.push_back(std::move(race));
    return weeks;
}

// ---- 5K schedule (Table 5.1) ----

static std::vector<WeekPlan> k5_schedule(const Paces& p) {
    using V = std::vector<Seg>;
    const Paces& q = p;

    auto r = [&](int n, int d, const std::string& ri) {
        return rep_str(n, d, rep_time(q, d), ri);
    };

    std::vector<WeekPlan> weeks;
    auto add = [&](int w, std::string kr1_txt, std::vector<PlanStep> kr1_s,
                   V kr2segs, float mi, PaceCode base, int off) {
        WeekPlan wp;
        wp.week      = w;
        wp.kr1       = kr1_txt;
        wp.kr2       = kr2_desc(kr2segs, q);
        wp.kr3       = kr3_desc(mi, base, off, q);
        wp.kr1_steps = kr1_s;
        wp.kr2_steps = kr2_steps(kr2segs, q);
        wp.kr3_steps = kr3_steps_fn(mi, base, off, q);
        weeks.push_back(std::move(wp));
    };

    auto ri400 = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),true,400); };
    auto ri90s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,90); };
    auto ri60s = [&](int n, int d) { return repeat_steps(n,d,rep_time(q,d),false,60); };

    auto mixed_w10 = [&]() {
        std::vector<PlanStep> s;
        auto tmp = repeat_steps(2,1600,q.r1600,true,400);
        s.insert(s.end(), tmp.begin(), tmp.end());
        s.push_back(run_step(800,q.r800,"800m"));
        return s;
    };
    auto ladder_w9 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i]; char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };
    auto desc_w7 = [&]() {
        std::vector<PlanStep> s;
        std::vector<std::pair<int,int>> dists = {{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}};
        for (size_t i = 0; i < dists.size(); i++) {
            auto [d,t] = dists[i]; char lbl[32]; snprintf(lbl,sizeof(lbl),"%dm",d);
            s.push_back(run_step(d,t,lbl));
            if (i < dists.size()-1) s.push_back(recover_dist(400));
        }
        return s;
    };

    add(12, r(8,400,"400m"), ri400(8,400),  {{2,ST}}, 5, HMP, 0);
    add(11, r(5,800,"400m"), ri400(5,800),  {{3,ST}}, 6, HMP, 0);
    add(10, "2x1600 (400m RI) + 1x800 (400m RI) @ " + fmt_time(q.r1600) + "/" + fmt_time(q.r800),
        mixed_w10(), {{1,EASY},{2,ST},{1,EASY},{2,ST}}, 5, HMP, 0);
    add(9,  rep_str_v({{400,q.r400},{600,q.r600},{800,q.r800},{800,q.r800},{600,q.r600},{400,q.r400}},"400m"),
        ladder_w9(), {{4,MT}}, 6, HMP, 0);
    add(8,  r(4,1000,"400m"), ri400(4,1000), {{3,ST}}, 7, HMP, 0);
    add(7,  rep_str_v({{1600,q.r1600},{1200,q.r1200},{800,q.r800},{400,q.r400}},"400m"),
        desc_w7(), {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 6, HMP, 0);
    add(6,  r(10,400,"90 sec"), ri90s(10,400), {{4,MT}}, 8, HMP, 0);
    add(5,  r(6,800,"90 sec"),  ri90s(6,800),  {{2,EASY},{3,ST}}, 7, HMP, 0);
    add(4,  r(4,1200,"400m"),   ri400(4,1200), {{3,ST}}, 7, HMP, 0);
    add(3,  r(5,1000,"400m"),   ri400(5,1000), {{1,EASY},{1,ST},{1,EASY},{1,ST},{1,EASY}}, 7, HMP, 0);
    add(2,  r(3,1600,"400m"),   ri400(3,1600), {{3,ST},{3,EASY}}, 6, HMP, 0);

    WeekPlan race;
    race.week = 1;
    race.kr1 = r(6,400,"60 sec"); race.kr1_steps = ri60s(6,400);
    race.kr2 = "3 mi easy (no extra warmup/cooldown)"; race.kr2_steps = {easy_step(3.0)};
    race.kr3 = "5K Race 3.1 mi"; race.kr3_steps = {};
    weeks.push_back(std::move(race));
    return weeks;
}

// ---- Public API ----

TrainingPlan generate_plan(const std::string& goal, const std::string& distance) {
    if (distance == "5k-novice") {
        TrainingPlan plan;
        plan.header   = "FIRST 5K Novice Training Program (Table 3.1) — 12 Weeks";
        plan.distance = "5k-novice";
        plan.paces    = Paces{};
        plan.weeks    = k5_novice_schedule();
        return plan;
    }

    int goal_sec = parse_time(goal);
    if (goal_sec <= 0)
        throw std::runtime_error("invalid goal time: " + goal);

    if (distance == "5k-intermediate") {
        Paces p = lookup_paces(goal_sec);
        p.goal_mp_mile  = p.mp_mile;
        p.goal_hmp_mile = p.hmp_mile;
        char hdr[256];
        snprintf(hdr, sizeof(hdr),
                 "FIRST 5K Intermediate Training Program (Table 3.2) — 12 Weeks\n"
                 "Goal 5K: %s", fmt_time(goal_sec).c_str());
        TrainingPlan plan;
        plan.header   = hdr;
        plan.distance = "5k-intermediate";
        plan.paces    = p;
        plan.weeks    = k5_intermediate_schedule(p);
        return plan;
    }

    int sk5 = goal_to_5k(goal_sec, distance);
    Paces p = lookup_paces(sk5);

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

    char hdr[256];
    const char* dist_label = distance == "5k"   ? "5K" :
                             distance == "10k"   ? "10K" :
                             distance == "half"  ? "Half-Marathon" : "Marathon";
    int weeks_count = (distance == "5k" || distance == "10k") ? 12 : 16;
    snprintf(hdr, sizeof(hdr),
             "FIRST Run Less Run Faster\n%d-Week %s Training Plan\n"
             "Goal: %s  |  Equiv 5K: %s",
             weeks_count, dist_label,
             fmt_time(goal_sec).c_str(), fmt_time(sk5).c_str());

    TrainingPlan plan;
    plan.header   = hdr;
    plan.distance = distance;
    plan.paces    = p;

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
    printf("%-6s  %-54s  %-44s  %s\n", "WEEK",
           "KEY RUN #1 (TRACK)", "KEY RUN #2 (TEMPO)", "KEY RUN #3 (LONG)");
    printf("%s\n", std::string(170, '-').c_str());

    for (const auto& w : plan.weeks) {
        if (w.week == 1) printf("\n");
        printf("%-6d  %-54s  %-44s  %s\n",
               w.week, w.kr1.c_str(), w.kr2.c_str(), w.kr3.c_str());
    }
    printf("\n");
}

void print_paces(const TrainingPlan& plan) {
    const Paces& p = plan.paces;

    if (p.r400 == 0 && p.st_mile == 0) {
        printf("No pace targets for this plan (novice walk/run program).\n");
        return;
    }

    printf("%s\n\n", plan.header.c_str());

    auto interval_pace = [](int dist_m, int time_sec) -> int {
        return (int)std::round((double)time_sec * 1609.344 / dist_m);
    };

    struct IV { int dist_m; int time_sec; };
    const IV ivs[] = {
        {400, p.r400}, {600, p.r600}, {800, p.r800}, {1000, p.r1000},
        {1200, p.r1200}, {1600, p.r1600}, {2000, p.r2000}
    };

    printf("Key Run #1 — Track Intervals\n");
    for (const auto& iv : ivs) {
        if (iv.time_sec == 0) continue;
        printf("  %4dm:  %s  (%s)\n", iv.dist_m,
               fmt_time(iv.time_sec).c_str(),
               fmt_pace(interval_pace(iv.dist_m, iv.time_sec)).c_str());
    }

    printf("\nKey Run #2 — Tempo Paces\n");
    printf("  Short tempo (ST):  %s\n", fmt_pace(p.st_mile).c_str());
    printf("  Mid tempo   (MT):  %s\n", fmt_pace(p.mt_mile).c_str());
    printf("  Long tempo  (LT):  %s\n", fmt_pace(p.lt_mile).c_str());

    if (p.goal_mp_mile > 0 || p.goal_hmp_mile > 0) {
        printf("\nKey Run #3 — Long Run Paces\n");
        if (p.goal_mp_mile  > 0) printf("  Marathon pace       (MP):   %s\n", fmt_pace(p.goal_mp_mile).c_str());
        if (p.goal_hmp_mile > 0) printf("  Half-marathon pace  (HMP):  %s\n", fmt_pace(p.goal_hmp_mile).c_str());
    }
    printf("\n");
}

Plan::Plan(const std::string& goal, const std::string& distance)
    : plan_(generate_plan(goal, distance)) {}

void Plan::print()      const { print_plan(plan_); }
void Plan::printPaces() const { print_paces(plan_); }

} // namespace first
