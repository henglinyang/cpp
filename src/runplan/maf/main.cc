#include "maf.h"
#include "src/fit2tcx/fit2tcx.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

static constexpr uint8_t kDurTime = 0;
static constexpr uint8_t kDurDist = 1;
static constexpr uint8_t kDurOpen = 5;

static void usage(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options]\n"
        "\n"
        "Generate a MAF workout: 15-min warmup + MAF run + 10-min cooldown.\n"
        "\n"
        "Options:\n"
        "  --age <N>          Athlete age (default: 50). maf_hr = 180 - age.\n"
        "  --distance <d>     Run distance: meters, or 5k/10k/half/marathon/full\n"
        "                   (default: lap-button press in run mode, 1mi in test mode)\n"
        "  --duration <s>     Run duration in seconds; overrides --distance\n"
        "  --test             MAF test mode: N timed laps to track split progress\n"
        "  --laps <N>         Number of test laps (default: 3, implies --test)\n"
        "  --tcx <file>       Write workout as TCX to file\n"
        "  --json <file>      Write workout as Garmin Connect JSON to file\n"
        "  -h, --help         Show this help\n"
        "\n"
        "Each run step targets HR in [maf_hr-8, maf_hr]. Warmup/cooldown use\n"
        "5 progressive steps walking HR between 90 and maf_hr.\n"
        "\n"
        "Examples:\n"
        "  %s --age 40                              # MAF run, lap-button end\n"
        "  %s --age 40 --distance 8000              # MAF run, 8K\n"
        "  %s --age 40 --test                       # MAF test, 3 x 1mi\n"
        "  %s --age 40 --laps 5                     # MAF test, 5 x 1mi\n"
        "  %s --age 40 --laps 1 --distance 5000     # MAF test, 1 x 5K\n",
        prog, prog, prog, prog, prog, prog);
}

int main(int argc, char* argv[]) {
    int  age        = 50;
    bool test_mode  = false;
    int  laps       = 0;
    int  distance_m = 0;
    int  duration_s = 0;
    std::string tcx_file;
    std::string json_file;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--test")) {
            test_mode = true;
        } else if (!strcmp(argv[i], "--laps") && i+1 < argc) {
            laps = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--distance") && i+1 < argc) {
            const char* d = argv[++i];
            if      (!strcmp(d, "5k"))                      distance_m = 5000;
            else if (!strcmp(d, "10k"))                     distance_m = 10000;
            else if (!strcmp(d, "half"))                    distance_m = 21097;
            else if (!strcmp(d, "marathon") || !strcmp(d, "full")) distance_m = 42195;
            else                                            distance_m = std::stoi(d);
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

    if (laps > 0 && !test_mode) {
        fprintf(stderr, "error: --laps requires --test\n");
        return 1;
    }
    if (distance_m > 0 && duration_s > 0) {
        fprintf(stderr, "error: specify at most one of --distance or --duration\n");
        return 1;
    }
    if (laps < 0) {
        fprintf(stderr, "error: --laps must be >= 1\n");
        return 1;
    }
    if (test_mode && laps == 0) laps = 3;

    uint8_t  run_dur_type;
    uint32_t run_dur_val;
    if (duration_s > 0) {
        run_dur_type = kDurTime;
        run_dur_val  = static_cast<uint32_t>(duration_s) * 1000u;
    } else if (distance_m > 0) {
        run_dur_type = kDurDist;
        run_dur_val  = static_cast<uint32_t>(distance_m) * 100u;
    } else if (test_mode) {
        run_dur_type = kDurDist;
        run_dur_val  = 1609u * 100u;
    } else {
        run_dur_type = kDurOpen;
        run_dur_val  = 0;
    }

    const int effective_laps = test_mode ? laps : 1;
    const int maf_hr = 180 - age;
    if (laps > 0)
        fprintf(stderr, "MAF test   age=%d  maf_hr=%d  laps=%d  run_hr=[%d,%d]\n",
                age, maf_hr, laps, maf_hr - 8, maf_hr);
    else
        fprintf(stderr, "MAF run    age=%d  maf_hr=%d  run_hr=[%d,%d]\n",
                age, maf_hr, maf_hr - 8, maf_hr);

    try {
        MafWorkout wkt(age, effective_laps, run_dur_type, run_dur_val);

        if (!tcx_file.empty()) {
            std::ofstream ofs(tcx_file);
            if (!ofs) throw std::runtime_error("cannot open: " + tcx_file);
            writeWorkoutTcx(wkt.data(), ofs);
        }
        if (!json_file.empty()) {
            std::ofstream ofs(json_file);
            if (!ofs) throw std::runtime_error("cannot open: " + json_file);
            writeWorkoutJson(wkt.data(), ofs);
            ofs << "\n";
        }
        if (tcx_file.empty() && json_file.empty())
            writeWorkoutJson(wkt.data(), std::cout);

    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
