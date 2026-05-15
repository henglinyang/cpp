#include "plan.h"
#include "tcx_export.h"

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --program <beginner|advanced>       (default: beginner)\n"
        "  --goal <H:MM:SS>                    marathon goal time (default: 3:30:00)\n"
        "  --tcx <dir>                         export SOS workouts as TCX files\n"
        "  --json <dir>                        export SOS workouts as Garmin JSON files\n"
        "  --age <N>                           athlete age for MAF HR (default: 50)\n"
        "  -h, --help\n\n"
        "Examples:\n"
        "  %s --program beginner --goal 4:00:00\n"
        "  %s --program advanced --goal 3:15:00 --json ./json --age 40\n",
        argv0, argv0, argv0);
}

int main(int argc, char* argv[]) {
    std::string program_str = "beginner";
    std::string goal        = "3:30:00";
    std::string tcx_dir;
    std::string json_dir;
    int age = 50;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
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
            usage(argv[0]);
            return 1;
        }
    }

    hanson::Program prog;
    if (program_str == "beginner")       prog = hanson::Program::BEGINNER;
    else if (program_str == "advanced")  prog = hanson::Program::ADVANCED;
    else {
        fprintf(stderr, "invalid program: %s (must be beginner or advanced)\n",
                program_str.c_str());
        return 1;
    }

    try {
        auto plan = hanson::generate_plan(goal, prog);
        hanson::print_plan(plan);
        if (!tcx_dir.empty()) {
            mkdir(tcx_dir.c_str(), 0755);
            hanson::export_plan_to_tcx(plan, tcx_dir, age);
        }
        if (!json_dir.empty()) {
            mkdir(json_dir.c_str(), 0755);
            hanson::export_plan_to_json(plan, json_dir, age);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
