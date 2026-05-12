#include "fit2tcx.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>

static void printHelp(const char* prog) {
    fprintf(stdout,
        "Usage: %s [options] <input.fit>\n"
        "\n"
        "Convert a Garmin FIT activity file to TCX (Training Center XML) format.\n"
        "\n"
        "Options:\n"
        "  -o <file>   Write TCX output to <file> instead of stdout.\n"
        "  -h, --help  Show this help message and exit.\n"
        "\n"
        "Examples:\n"
        "  %s activity.fit                   # print TCX to stdout\n"
        "  %s activity.fit -o activity.tcx   # write TCX to file\n"
        "  %s -o activity.tcx activity.fit   # options may precede the input\n",
        prog, prog, prog, prog);
}

int main(int argc, char* argv[]) {
    const char* inputPath  = nullptr;
    const char* outputPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printHelp(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -o requires a filename argument\n");
                return 1;
            }
            outputPath = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
            return 1;
        } else {
            if (inputPath) {
                fprintf(stderr, "error: unexpected argument '%s'\n", argv[i]);
                fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
                return 1;
            }
            inputPath = argv[i];
        }
    }

    if (!inputPath) {
        fprintf(stderr, "error: no input file specified\n");
        fprintf(stderr, "Run '%s --help' for usage.\n", argv[0]);
        return 1;
    }

    std::fstream inFile(inputPath, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) {
        fprintf(stderr, "error: cannot open '%s'\n", inputPath);
        return 1;
    }

    try {
        FitData data = decodeFit(inFile);

        if (outputPath) {
            std::ofstream outFile(outputPath);
            if (!outFile.is_open()) {
                fprintf(stderr, "error: cannot open output file '%s'\n", outputPath);
                return 1;
            }
            writeTcx(data.records, data.laps, data.session, data.has_session, outFile);
        } else {
            writeTcx(data.records, data.laps, data.session, data.has_session, std::cout);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
