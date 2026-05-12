#include "fit2tcx.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: fit2tcx <filename.fit>\n");
        return 1;
    }
    std::fstream file(argv[1], std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        return 1;
    }
    try {
        FitData data = decodeFit(file);
        writeTcx(data.records, data.laps, data.session, data.has_session, std::cout);
    } catch (const std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    return 0;
}
