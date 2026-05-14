#include "plan.h"
#include "tcx_export.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --distance <5k|10k|half|marathon>   (default: marathon)\n"
        "  --goal <H:MM:SS or M:SS>             goal finish time (default: 3:15:00)\n"
        "  --tcx <dir>                          export workouts as TCX files into dir\n"
        "  --age <N>                            athlete age for MAF HR (default: 35)\n"
        "  -h, --help\n\n"
        "Examples:\n"
        "  %s --distance marathon --goal 3:15:00\n"
        "  %s --distance marathon --goal 3:15:00 --tcx ./tcx --age 40\n"
        "  %s --distance half     --goal 1:35:00\n"
        "  %s --distance 10k      --goal 45:00\n"
        "  %s --distance 5k       --goal 20:00\n",
        argv0, argv0, argv0, argv0, argv0, argv0);
}

int main(int argc, char* argv[]) {
    std::string distance = "marathon";
    std::string goal     = "3:15:00";
    std::string tcx_dir;
    int age = 35;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "--distance") && i+1 < argc) {
            distance = argv[++i];
        } else if (!strcmp(argv[i], "--goal") && i+1 < argc) {
            goal = argv[++i];
        } else if (!strcmp(argv[i], "--tcx") && i+1 < argc) {
            tcx_dir = argv[++i];
        } else if (!strcmp(argv[i], "--age") && i+1 < argc) {
            age = std::stoi(argv[++i]);
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (distance != "5k" && distance != "10k" &&
        distance != "half" && distance != "marathon") {
        fprintf(stderr, "invalid distance: %s (must be 5k, 10k, half, marathon)\n",
                distance.c_str());
        return 1;
    }

    try {
        auto plan = first::generate_plan(goal, distance);
        first::print_plan(plan);
        if (!tcx_dir.empty()) {
            mkdir(tcx_dir.c_str(), 0755);
            first::export_plan_to_tcx(plan, tcx_dir, age);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
