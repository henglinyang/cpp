#include "maf.h"
#include "src/fit2tcx/fit2tcx.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static constexpr uint8_t kDurTime     = 0;
static constexpr uint8_t kDurDist     = 1;
static constexpr uint8_t kDurOpen     = 5;
static constexpr uint8_t kTgtHr       = 1;
static constexpr uint8_t kInActive    = 0;
static constexpr uint8_t kSportRunning = 1;

static void usage(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options]\n"
        "\n"
        "Generate a MAF heart-rate workout: 15-min warmup + MAF run + 10-min cooldown.\n"
        "\n"
        "Options:\n"
        "  --age <N>          Athlete age (default: 50). maf_hr = 180 - age.\n"
        "  --distance <m>     Main run distance in meters (default: open duration)\n"
        "  --duration <s>     Main run duration in seconds (default: open duration)\n"
        "  --tcx <file>       Write workout as TCX to file\n"
        "  --json <file>      Write workout as Garmin Connect JSON to file\n"
        "  -h, --help         Show this help\n"
        "\n"
        "The MAF run step targets HR in [maf_hr-8, maf_hr]. Warmup/cooldown use\n"
        "5 progressive steps walking HR between 90 and maf_hr.\n"
        "\n"
        "Examples:\n"
        "  %s --age 40 --distance 1609 --json maf.json\n"
        "  %s --age 35 --duration 3600 --tcx maf.tcx\n"
        "  %s --age 38 --json maf.json --tcx maf.tcx\n",
        prog, prog, prog, prog);
}

static WorkoutData build_maf_workout(int age, uint8_t run_dur_type,
                                      uint32_t run_dur_val) {
    const int maf_hr = 180 - age;

    WorkoutData wkt;
    wkt.has_name  = true;
    wkt.name      = "MAF";
    wkt.has_sport = true;
    wkt.sport     = kSportRunning;

    uint16_t idx = 0;
    push_maf_warmup(wkt, idx, 900, maf_hr);

    WorkoutStepData run;
    run.step_index      = idx++;
    run.duration_type   = run_dur_type;
    run.duration_value  = run_dur_val;
    run.intensity       = kInActive;
    run.target_type     = kTgtHr;
    run.has_target_low  = true;
    run.target_low      = static_cast<uint32_t>(maf_hr - 8);
    run.has_target_high = true;
    run.target_high     = static_cast<uint32_t>(maf_hr);
    wkt.steps.push_back(run);

    push_maf_cooldown(wkt, idx, 600, maf_hr);
    return wkt;
}

int main(int argc, char* argv[]) {
    int age = 50;
    int distance_m  = 0;
    int duration_s  = 0;
    std::string tcx_file;
    std::string json_file;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--distance") && i+1 < argc) {
            distance_m = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--duration") && i+1 < argc) {
            duration_s = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--tcx") && i+1 < argc) {
            tcx_file = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i+1 < argc) {
            json_file = argv[++i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage(argv[0]); return 1;
        }
    }

    if (distance_m > 0 && duration_s > 0) {
        fprintf(stderr, "error: specify at most one of --distance or --duration\n");
        return 1;
    }

    uint8_t  run_dur_type;
    uint32_t run_dur_val;
    if (distance_m > 0) {
        run_dur_type = kDurDist;
        run_dur_val  = static_cast<uint32_t>(distance_m) * 100u; // m → cm
    } else if (duration_s > 0) {
        run_dur_type = kDurTime;
        run_dur_val  = static_cast<uint32_t>(duration_s) * 1000u; // s → ms
    } else {
        run_dur_type = kDurOpen;
        run_dur_val  = 0;
    }

    const int maf_hr = 180 - age;
    fprintf(stderr, "MAF workout  age=%d  maf_hr=%d  run_hr=[%d, %d]\n",
            age, maf_hr, maf_hr - 8, maf_hr);

    try {
        WorkoutData wkt = build_maf_workout(age, run_dur_type, run_dur_val);

        if (!tcx_file.empty()) {
            std::ofstream ofs(tcx_file);
            if (!ofs) throw std::runtime_error("cannot open: " + tcx_file);
            writeWorkoutTcx(wkt, ofs);
        }
        if (!json_file.empty()) {
            std::ofstream ofs(json_file);
            if (!ofs) throw std::runtime_error("cannot open: " + json_file);
            writeWorkoutJson(wkt, ofs);
            ofs << "\n";
        }
        if (tcx_file.empty() && json_file.empty())
            writeWorkoutJson(wkt, std::cout);

    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
