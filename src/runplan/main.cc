#include "src/fit2tcx/fit2tcx.h"
#include "src/runplan/first/plan.h"
#include "src/runplan/hanson/plan.h"
#include "src/runplan/maf/maf.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

static constexpr uint8_t kDurTime = 0;
static constexpr uint8_t kDurDist = 1;
static constexpr uint8_t kDurOpen = 5;

// ---- top-level help --------------------------------------------------------

static void usage_top(const char* prog) {
    fprintf(stderr,
        "Usage: %s <command> [options]\n"
        "\n"
        "Commands:\n"
        "  maf      Generate a MAF workout (15-min warmup + run + 10-min cooldown)\n"
        "  hanson   Generate an 18-week Hansons marathon training plan\n"
        "  first    Generate a FIRST training plan (5k/10k/half/marathon)\n"
        "\n"
        "Run '%s <command> --help' for command-specific options.\n",
        prog, prog);
}

// ---- maf -------------------------------------------------------------------

static void usage_maf(const char* prog) {
    fprintf(stdout,
        "Usage: %s maf [options]\n"
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
        "  %s maf --age 40\n"
        "  %s maf --age 40 --distance 8000\n"
        "  %s maf --age 40 --test\n"
        "  %s maf --age 40 --laps 5\n"
        "  %s maf --age 40 --laps 1 --distance 5000\n",
        prog, prog, prog, prog, prog, prog);
}

static int cmd_maf(const char* prog, int argc, char* argv[]) {
    int  age        = 50;
    bool test_mode  = false;
    int  laps       = 0;
    int  distance_m = 0;
    int  duration_s = 0;
    std::string tcx_file;
    std::string json_file;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage_maf(prog); return 0;
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--test")) {
            test_mode = true;
        } else if (!strcmp(argv[i], "--laps") && i+1 < argc) {
            laps = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--distance") && i+1 < argc) {
            const char* d = argv[++i];
            if      (!strcmp(d, "5k"))                           distance_m = 5000;
            else if (!strcmp(d, "10k"))                          distance_m = 10000;
            else if (!strcmp(d, "half"))                         distance_m = 21097;
            else if (!strcmp(d, "marathon") || !strcmp(d, "full")) distance_m = 42195;
            else                                                 distance_m = std::stoi(d);
        } else if (!strcmp(argv[i], "--duration") && i+1 < argc) {
            duration_s = std::stoi(argv[++i]);
        } else if (!strcmp(argv[i], "--tcx") && i+1 < argc) {
            tcx_file = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i+1 < argc) {
            json_file = argv[++i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage_maf(prog); return 1;
        }
    }

    if (laps > 0 && !test_mode) {
        fprintf(stderr, "error: --laps requires --test\n"); return 1;
    }
    if (distance_m > 0 && duration_s > 0) {
        fprintf(stderr, "error: specify at most one of --distance or --duration\n"); return 1;
    }
    if (laps < 0) {
        fprintf(stderr, "error: --laps must be >= 1\n"); return 1;
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
        fprintf(stderr, "error: %s\n", e.what()); return 1;
    }
    return 0;
}

// ---- hanson ----------------------------------------------------------------

static void usage_hanson(const char* prog) {
    fprintf(stderr,
        "Usage: %s hanson [options]\n"
        "\n"
        "Generate an 18-week Hansons Marathon Method training plan.\n"
        "\n"
        "Options:\n"
        "  --program <beginner|advanced>   (default: beginner)\n"
        "  --goal <H:MM:SS>                Marathon goal time (default: 3:30:00)\n"
        "  --tcx <dir>                     Export SOS workouts as TCX files\n"
        "  --json <dir>                    Export SOS workouts as Garmin JSON files\n"
        "  --age <N>                       Athlete age for MAF HR (default: 50)\n"
        "  -h, --help\n"
        "\n"
        "Examples:\n"
        "  %s hanson --program beginner --goal 4:00:00\n"
        "  %s hanson --program advanced --goal 3:15:00 --json ./json --age 40\n",
        prog, prog, prog);
}

static int cmd_hanson(const char* prog, int argc, char* argv[]) {
    std::string program_str = "beginner";
    std::string goal        = "3:30:00";
    std::string tcx_dir;
    std::string json_dir;
    int age = 50;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage_hanson(prog); return 0;
        } else if (!strcmp(argv[i], "--program") && i+1 < argc) {
            program_str = argv[++i];
        } else if (!strcmp(argv[i], "--goal") && i+1 < argc) {
            goal = argv[++i];
        } else if (!strcmp(argv[i], "--tcx") && i+1 < argc) {
            tcx_dir = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i+1 < argc) {
            json_dir = argv[++i];
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage_hanson(prog); return 1;
        }
    }

    hanson::Program prog_enum;
    if (program_str == "beginner")      prog_enum = hanson::Program::BEGINNER;
    else if (program_str == "advanced") prog_enum = hanson::Program::ADVANCED;
    else {
        fprintf(stderr, "invalid program: %s (must be beginner or advanced)\n",
                program_str.c_str());
        return 1;
    }

    try {
        hanson::Plan plan(goal, prog_enum);
        plan.print();
        if (!tcx_dir.empty()) { mkdir(tcx_dir.c_str(), 0755); plan.exportTcx(tcx_dir, age); }
        if (!json_dir.empty()) { mkdir(json_dir.c_str(), 0755); plan.exportJson(json_dir, age); }
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what()); return 1;
    }
    return 0;
}

// ---- first -----------------------------------------------------------------

static void usage_first(const char* prog) {
    fprintf(stderr,
        "Usage: %s first [options]\n"
        "\n"
        "Generate a FIRST (Furman Institute) training plan.\n"
        "\n"
        "Options:\n"
        "  --distance <5k|10k|half|marathon>   (default: marathon)\n"
        "  --goal <H:MM:SS or M:SS>             Goal finish time (default: 3:15:00)\n"
        "  --tcx <dir>                          Export workouts as TCX files\n"
        "  --json <dir>                         Export workouts as Garmin JSON files\n"
        "  --age <N>                            Athlete age for MAF HR (default: 50)\n"
        "  -h, --help\n"
        "\n"
        "Examples:\n"
        "  %s first --distance marathon --goal 3:15:00\n"
        "  %s first --distance half --goal 1:35:00 --json ./json --age 40\n"
        "  %s first --distance 10k --goal 45:00\n"
        "  %s first --distance 5k --goal 20:00\n",
        prog, prog, prog, prog, prog);
}

static int cmd_first(const char* prog, int argc, char* argv[]) {
    std::string distance = "marathon";
    std::string goal     = "3:15:00";
    std::string tcx_dir;
    std::string json_dir;
    int age = 50;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage_first(prog); return 0;
        } else if (!strcmp(argv[i], "--distance") && i+1 < argc) {
            distance = argv[++i];
        } else if (!strcmp(argv[i], "--goal") && i+1 < argc) {
            goal = argv[++i];
        } else if (!strcmp(argv[i], "--tcx") && i+1 < argc) {
            tcx_dir = argv[++i];
        } else if (!strcmp(argv[i], "--json") && i+1 < argc) {
            json_dir = argv[++i];
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage_first(prog); return 1;
        }
    }

    if (distance != "5k" && distance != "10k" &&
        distance != "half" && distance != "marathon") {
        fprintf(stderr, "invalid distance: %s (must be 5k, 10k, half, marathon)\n",
                distance.c_str());
        return 1;
    }

    try {
        first::Plan plan(goal, distance);
        plan.print();
        if (!tcx_dir.empty()) { mkdir(tcx_dir.c_str(), 0755); plan.exportTcx(tcx_dir, age); }
        if (!json_dir.empty()) { mkdir(json_dir.c_str(), 0755); plan.exportJson(json_dir, age); }
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what()); return 1;
    }
    return 0;
}

// ---- main ------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")
                 || !strcmp(argv[1], "help")) {
        usage_top(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    const char* cmd = argv[1];
    if (!strcmp(cmd, "maf"))    return cmd_maf(argv[0], argc - 1, argv + 1);
    if (!strcmp(cmd, "hanson")) return cmd_hanson(argv[0], argc - 1, argv + 1);
    if (!strcmp(cmd, "first"))  return cmd_first(argv[0], argc - 1, argv + 1);

    fprintf(stderr, "%s: unknown command '%s'\n\n", argv[0], cmd);
    usage_top(argv[0]);
    return 1;
}
