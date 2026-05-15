#include "tables.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace hanson {

// kT35: 27 rows × 10 cols
// col 0: marathon goal (sec), descending
// col 1: half-marathon goal (sec)
// col 2: easy_a (sec/mile, slowest easy)
// col 3: easy_b (sec/mile)
// col 4: easy_c (sec/mile)
// col 5: moderate_long (sec/mile, long run)
// col 6: tempo / marathon goal pace (sec/mile)
// col 7: strength workout pace (sec/mile)
// col 8: 10K pace (sec/mile)
// col 9: 5K pace (sec/mile)
static const int kT35[][10] = {
    {18000, 8640, 862, 812, 761, 736, 687, 677, 630, 604},
    {17100, 8220, 823, 775, 725, 701, 652, 642, 598, 574},
    {16200, 7800, 782, 736, 688, 665, 618, 608, 567, 544},
    {15300, 7320, 742, 698, 652, 629, 584, 574, 535, 513},
    {14400, 6900, 702, 660, 615, 593, 549, 539, 504, 483},
    {14100, 6780, 688, 640, 600, 578, 538, 528, 493, 473},
    {13800, 6600, 675, 634, 591, 569, 526, 516, 483, 463},
    {13500, 6480, 661, 621, 579, 558, 515, 505, 472, 453},
    {13200, 6300, 648, 608, 567, 546, 503, 493, 462, 443},
    {12900, 6180, 634, 595, 554, 533, 492, 482, 451, 433},
    {12600, 6060, 619, 581, 542, 522, 481, 471, 441, 423},
    {12300, 5880, 606, 568, 529, 509, 469, 459, 430, 413},
    {12000, 5760, 593, 556, 518, 498, 458, 448, 420, 403},
    {11700, 5610, 578, 542, 505, 485, 446, 436, 409, 393},
    {11400, 5460, 565, 529, 493, 474, 435, 425, 399, 383},
    {11100, 5340, 551, 516, 481, 462, 423, 413, 388, 372},
    {10800, 5160, 537, 503, 468, 449, 412, 402, 378, 362},
    {10500, 5040, 523, 490, 456, 437, 400, 390, 367, 352},
    {10200, 4890, 508, 476, 443, 425, 389, 379, 357, 342},
    {9900, 4740, 495, 463, 431, 413, 378, 368, 346, 332},
    {9600, 4620, 480, 450, 418, 401, 366, 356, 336, 322},
    {9300, 4440, 466, 437, 406, 389, 355, 345, 325, 312},
    {9000, 4320, 452, 423, 394, 377, 343, 333, 315, 302},
    {8700, 4170, 438, 410, 381, 365, 332, 322, 304, 292},
    {8400, 4020, 423, 396, 368, 352, 320, 310, 294, 282},
    {8100, 3885, 409, 383, 356, 340, 309, 299, 283, 272},
    {7800, 3750, 395, 369, 343, 328, 297, 287, 273, 262},
};

// kR400: 28 rows × 3 cols (only first 26 rows used; last 2 overlap with kR600)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 400m repeat time (sec) — output
static const int kR400[][3] = {
    {930, 1950, 75},
    {960, 2015, 78},
    {990, 2080, 80},
    {1020, 2145, 83},
    {1050, 2210, 85},
    {1080, 2275, 88},
    {1110, 2340, 90},
    {1140, 2405, 93},
    {1170, 2470, 95},
    {1200, 2535, 98},
    {1230, 2600, 100},
    {1260, 2665, 103},
    {1290, 2730, 105},
    {1320, 2795, 108},
    {1350, 2860, 110},
    {1380, 2925, 113},
    {1410, 2990, 115},
    {1440, 3055, 118},
    {1470, 3120, 121},
    {1500, 3185, 123},
    {1530, 3250, 126},
    {1560, 3315, 128},
    {1620, 3445, 133},
    {1680, 3585, 138},
    {1740, 3725, 143},
    {1800, 3865, 148},
    {930, 1950, 112},
    {960, 2015, 115},
};

// kR600: 28 rows × 3 cols (only first 26 rows used; last 2 overlap with kR800)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 600m repeat time (sec) — output
static const int kR600[][3] = {
    {930, 1950, 112},
    {960, 2015, 115},
    {990, 2080, 119},
    {1020, 2145, 123},
    {1050, 2210, 126},
    {1080, 2275, 130},
    {1110, 2340, 134},
    {1140, 2405, 138},
    {1170, 2470, 141},
    {1200, 2535, 145},
    {1230, 2600, 149},
    {1260, 2665, 153},
    {1290, 2730, 156},
    {1320, 2795, 160},
    {1350, 2860, 164},
    {1380, 2925, 168},
    {1410, 2990, 171},
    {1440, 3055, 175},
    {1470, 3120, 179},
    {1500, 3185, 183},
    {1530, 3250, 186},
    {1560, 3315, 190},
    {1620, 3445, 197},
    {1680, 3585, 203},
    {1740, 3725, 210},
    {1800, 3865, 216},
    {930, 1950, 150},
    {960, 2015, 155},
};

// kR800: 28 rows × 3 cols (only first 26 rows used; last 2 overlap with kR1K)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 800m repeat time (sec) — output
static const int kR800[][3] = {
    {930, 1950, 150},
    {960, 2015, 155},
    {990, 2080, 160},
    {1020, 2145, 165},
    {1050, 2210, 170},
    {1080, 2275, 175},
    {1110, 2340, 180},
    {1140, 2405, 185},
    {1170, 2470, 190},
    {1200, 2535, 195},
    {1230, 2600, 200},
    {1260, 2665, 205},
    {1290, 2730, 210},
    {1320, 2795, 215},
    {1350, 2860, 220},
    {1380, 2925, 225},
    {1410, 2990, 230},
    {1440, 3055, 235},
    {1470, 3120, 240},
    {1500, 3185, 245},
    {1530, 3250, 250},
    {1560, 3315, 255},
    {1620, 3445, 265},
    {1680, 3585, 275},
    {1740, 3725, 285},
    {1800, 3865, 295},
    {930, 1950, 186},
    {960, 2015, 192},
};

// kR1K: 28 rows × 3 cols (only first 26 rows used; last 2 overlap with kR1200)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 1000m repeat time (sec) — output
static const int kR1K[][3] = {
    {930, 1950, 186},
    {960, 2015, 192},
    {990, 2080, 198},
    {1020, 2145, 204},
    {1050, 2210, 210},
    {1080, 2275, 216},
    {1110, 2340, 222},
    {1140, 2405, 228},
    {1170, 2470, 234},
    {1200, 2535, 240},
    {1230, 2600, 246},
    {1260, 2665, 252},
    {1290, 2730, 258},
    {1320, 2795, 264},
    {1350, 2860, 270},
    {1380, 2925, 276},
    {1410, 2990, 282},
    {1440, 3055, 288},
    {1470, 3120, 294},
    {1500, 3185, 300},
    {1530, 3250, 306},
    {1560, 3315, 312},
    {1620, 3445, 324},
    {1680, 3585, 336},
    {1740, 3725, 348},
    {1800, 3865, 360},
    {930, 1950, 222},
    {960, 2015, 230},
};

// kR1200: 28 rows × 3 cols (only first 26 rows used; last 2 overlap with kR1600)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 1200m repeat time (sec) — output
static const int kR1200[][3] = {
    {930, 1950, 222},
    {960, 2015, 230},
    {990, 2080, 237},
    {1020, 2145, 245},
    {1050, 2210, 252},
    {1080, 2275, 260},
    {1110, 2340, 267},
    {1140, 2405, 275},
    {1170, 2470, 282},
    {1200, 2535, 290},
    {1230, 2600, 297},
    {1260, 2665, 305},
    {1290, 2730, 312},
    {1320, 2795, 320},
    {1350, 2860, 327},
    {1380, 2925, 335},
    {1410, 2990, 342},
    {1440, 3055, 350},
    {1470, 3120, 357},
    {1500, 3185, 365},
    {1530, 3250, 372},
    {1560, 3315, 380},
    {1620, 3445, 396},
    {1680, 3585, 411},
    {1740, 3725, 427},
    {1800, 3865, 443},
    {930, 1950, 75},
    {150, 222, 300},
};

// kR1600: 28 rows × 3 cols (only first 26 rows used)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference, not used in interpolation
// col 2: 1600m repeat time (sec) — output
static const int kR1600[][3] = {
    {930, 1950, 300},
    {960, 2015, 310},
    {990, 2080, 320},
    {1020, 2145, 330},
    {1050, 2210, 340},
    {1080, 2275, 350},
    {1110, 2340, 360},
    {1140, 2405, 370},
    {1170, 2470, 380},
    {1200, 2535, 390},
    {1230, 2600, 400},
    {1260, 2665, 410},
    {1290, 2730, 420},
    {1320, 2795, 430},
    {1350, 2860, 440},
    {1380, 2925, 450},
    {1410, 2990, 460},
    {1440, 3055, 470},
    {1470, 3120, 480},
    {1500, 3185, 490},
    {1530, 3250, 500},
    {1560, 3315, 510},
    {1620, 3445, 530},
    {1680, 3585, 550},
    {1740, 3725, 570},
    {1800, 3865, 590},
    {8880, 4440, 330},
    {9180, 4590, 340},
};

// kLADDER: 28 rows × 6 cols (not used by lookup_paces; individual kR* tables used instead)
// col 0: 5K race time (sec, ascending) — lookup key
// col 1: 10K race time (sec) — reference
// col 2: 400m repeat time (sec)
// col 3: 800m repeat time (sec)
// col 4: 1200m repeat time (sec)
// col 5: 1600m repeat time (sec)
static const int kLADDER[][6] = {
    {930, 1950, 75, 150, 222, 300},
    {960, 2015, 78, 155, 230, 310},
    {990, 2080, 80, 160, 237, 320},
    {1020, 2145, 83, 165, 245, 330},
    {1050, 2210, 85, 170, 252, 340},
    {1080, 2275, 88, 174, 260, 350},
    {1110, 2340, 90, 179, 267, 360},
    {1140, 2405, 93, 184, 275, 370},
    {1170, 2470, 95, 189, 282, 380},
    {1200, 2535, 98, 194, 290, 390},
    {1230, 2600, 100, 199, 297, 400},
    {1260, 2665, 103, 204, 305, 410},
    {1290, 2730, 105, 209, 312, 420},
    {1320, 2795, 108, 214, 320, 430},
    {1350, 2860, 110, 219, 327, 440},
    {1380, 2925, 113, 224, 335, 450},
    {1410, 2990, 115, 229, 342, 460},
    {1440, 3055, 118, 234, 350, 470},
    {1470, 3120, 121, 239, 357, 480},
    {1500, 3185, 123, 244, 365, 490},
    {1530, 3250, 126, 249, 372, 500},
    {1560, 3315, 128, 254, 380, 510},
    {1620, 3445, 133, 265, 396, 530},
    {1680, 3585, 138, 275, 411, 550},
    {1740, 3725, 143, 285, 427, 570},
    {1800, 3865, 148, 295, 443, 590},
    {930, 1950, 300, 960, 2015, 310},
    {990, 2080, 320, 1020, 2145, 330},
};

// kS1MI: 36 rows × 3 cols
// col 0: marathon goal time (sec, ascending) — lookup key
// col 1: half-marathon goal time (sec) — reference, not used in interpolation
// col 2: 1-mile strength repeat time (sec) — output
static const int kS1MI[][3] = {
    {8880, 4440, 330},
    {9180, 4590, 340},
    {9480, 4740, 350},
    {9720, 4860, 360},
    {9960, 4980, 370},
    {10200, 5100, 380},
    {10500, 5250, 390},
    {10740, 5370, 400},
    {10980, 5490, 410},
    {11280, 5640, 420},
    {11520, 5760, 430},
    {11820, 5910, 440},
    {12060, 6030, 450},
    {12300, 6150, 460},
    {12600, 6300, 470},
    {12840, 6450, 480},
    {13080, 6540, 490},
    {13380, 6690, 500},
    {13620, 6810, 510},
    {13860, 6930, 520},
    {14160, 7080, 530},
    {14400, 7200, 540},
    {14640, 7320, 550},
    {14940, 7470, 560},
    {15180, 7590, 570},
    {15480, 7740, 580},
    {15720, 7860, 590},
    {15960, 7980, 600},
    {16260, 8130, 610},
    {16500, 8250, 620},
    {16740, 8370, 630},
    {17040, 8190, 640},
    {17280, 8640, 650},
    {17580, 8790, 660},
    {17820, 8910, 670},
    {18060, 9030, 680},
};

// kS15MI: 36 rows × 3 cols
// col 0: marathon goal time (sec, ascending) — lookup key
// col 1: half-marathon goal time (sec) — reference, not used in interpolation
// col 2: 1.5-mile strength repeat time (sec) — output
static const int kS15MI[][3] = {
    {8880, 4440, 495},
    {9180, 4590, 510},
    {9480, 4740, 525},
    {9720, 4860, 540},
    {9960, 4980, 555},
    {10200, 5100, 570},
    {10500, 5250, 585},
    {10740, 5370, 600},
    {10980, 5490, 615},
    {11280, 5640, 630},
    {11520, 5760, 645},
    {11820, 5910, 660},
    {12060, 6030, 675},
    {12300, 6150, 690},
    {12600, 6300, 705},
    {12840, 6450, 720},
    {13080, 6540, 735},
    {13380, 6690, 750},
    {13620, 6810, 765},
    {13860, 6930, 780},
    {14160, 7080, 795},
    {14400, 7200, 810},
    {14640, 7320, 825},
    {14940, 7470, 840},
    {15180, 7590, 855},
    {15480, 7740, 870},
    {15720, 7860, 885},
    {15960, 7980, 900},
    {16260, 8130, 915},
    {16500, 8250, 930},
    {16740, 8370, 945},
    {17040, 8190, 960},
    {17280, 8640, 975},
    {17580, 8790, 990},
    {17820, 8910, 1005},
    {18060, 9030, 1020},
};

// kS2MI: 36 rows × 3 cols (only first 20 rows used)
// col 0: marathon goal time (sec, ascending) — lookup key
// col 1: half-marathon goal time (sec) — reference, not used in interpolation
// col 2: 2-mile strength repeat time (sec) — output
static const int kS2MI[][3] = {
    {8880, 4440, 660},
    {9180, 4590, 680},
    {9480, 4740, 700},
    {9720, 4860, 720},
    {9960, 4980, 740},
    {10200, 5100, 760},
    {10500, 5250, 780},
    {10740, 5370, 800},
    {10980, 5490, 820},
    {11280, 5640, 840},
    {11520, 5760, 860},
    {11820, 5910, 880},
    {12060, 6030, 900},
    {12300, 6150, 920},
    {12600, 6300, 940},
    {12840, 6450, 960},
    {13080, 6540, 980},
    {13380, 6690, 1000},
    {13620, 6810, 1020},
    {13860, 6930, 1040},
    {14160, 7080, 1060},
    {14400, 7200, 1080},
    {14640, 7320, 1100},
    {14940, 7470, 1120},
    {15180, 7590, 1140},
    {15480, 7740, 1160},
    {15720, 7860, 1180},
    {15960, 7980, 1200},
    {16260, 8130, 1220},
    {16500, 8250, 1240},
    {16740, 8370, 1260},
    {17040, 8190, 1280},
    {17280, 8640, 1300},
    {17580, 8790, 1320},
    {17820, 8910, 1340},
    {18060, 9030, 1360},
};

// kS3MI: 36 rows × 3 cols
// col 0: marathon goal time (sec, ascending) — lookup key
// col 1: half-marathon goal time (sec) — reference, not used in interpolation
// col 2: 3-mile strength repeat time (sec) — output
static const int kS3MI[][3] = {
    {8880, 4440, 990},
    {9180, 4590, 1020},
    {9480, 4740, 1050},
    {9720, 4860, 1080},
    {9960, 4980, 1110},
    {10200, 5100, 1140},
    {10500, 5250, 1170},
    {10740, 5370, 1200},
    {10980, 5490, 1230},
    {11280, 5640, 1260},
    {11520, 5760, 1290},
    {11820, 5910, 1320},
    {12060, 6030, 1350},
    {12300, 6150, 1380},
    {12600, 6300, 1410},
    {12840, 6450, 1440},
    {13080, 6540, 1470},
    {13380, 6690, 1500},
    {13620, 6810, 1530},
    {13860, 6930, 1560},
    {14160, 7080, 1590},
    {14400, 7200, 1620},
    {14640, 7320, 1650},
    {14940, 7470, 1680},
    {15180, 7590, 1710},
    {15480, 7740, 1740},
    {15720, 7860, 1770},
    {15960, 7980, 1800},
    {16260, 8130, 1830},
    {16500, 8250, 1860},
    {16740, 8370, 1890},
    {17040, 8190, 1920},
    {17280, 8640, 1950},
    {17580, 8790, 1980},
    {17820, 8910, 2010},
    {18060, 9030, 2040},
};

// kTEMPO: 27 rows × 3 cols (not used by lookup_paces; tempo is read from kT35 col 6)
// col 0: marathon goal time (sec, descending) — lookup key
// col 1: half-marathon goal time (sec) — reference
// col 2: tempo / marathon goal pace (sec/mile) — output
static const int kTEMPO[][3] = {
    {18000, 8640, 687},
    {17100, 8220, 652},
    {16200, 7800, 618},
    {15300, 7320, 584},
    {14400, 6900, 549},
    {14100, 6780, 538},
    {13800, 6600, 526},
    {13500, 6480, 515},
    {13200, 6300, 503},
    {12900, 6180, 492},
    {12600, 6060, 481},
    {12300, 5880, 469},
    {12000, 5760, 458},
    {11700, 5610, 446},
    {11400, 5460, 435},
    {11100, 5340, 423},
    {10800, 5160, 412},
    {10500, 5040, 400},
    {10200, 4890, 389},
    {9900, 4740, 378},
    {9600, 4620, 366},
    {9300, 4440, 355},
    {9000, 4320, 343},
    {8700, 4170, 332},
    {8400, 4020, 320},
    {8100, 3885, 309},
    {7800, 3750, 297},
};

static const int kT35_rows = 27;
static const int kSpeed_rows = 26;
static const int kS_rows = 33;
static const int kS2MI_rows = 20;

int parse_time(const std::string& s) {
    int h = 0, m = 0, sec = 0;
    int cnt = 0;
    for (char c : s) if (c == ':') cnt++;
    if (cnt == 2) {
        sscanf(s.c_str(), "%d:%d:%d", &h, &m, &sec);
        return h * 3600 + m * 60 + sec;
    } else {
        sscanf(s.c_str(), "%d:%d", &m, &sec);
        return m * 60 + sec;
    }
}

std::string fmt_time(int sec) {
    if (sec < 0) return "?";
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    char buf[32];
    if (h > 0)
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else
        snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

std::string fmt_pace(int sec_per_mile) {
    if (sec_per_mile <= 0) return "?";
    return fmt_time(sec_per_mile) + "/mi";
}

static int lerp(int a, int b, double t) {
    return static_cast<int>(std::round(a + t * (b - a)));
}

// Interpolate column `col` from a table with `ncols` columns, using key column 0.
// Assumes key column is monotonically increasing (ascending).
static int interp_asc(const int* table, int nrows, int ncols, int key, int col) {
    for (int i = 0; i < nrows - 1; i++) {
        int a = table[i * ncols], b = table[(i + 1) * ncols];
        if (key >= a && key <= b) {
            double frac = static_cast<double>(key - a) / (b - a);
            return lerp(table[i * ncols + col], table[(i + 1) * ncols + col], frac);
        }
    }
    if (key <= table[0]) return table[col];
    return table[(nrows - 1) * ncols + col];
}

// Interpolate column `col` from a table with `ncols` columns, using key column 0.
// Assumes key column is monotonically decreasing (descending).
static int interp_desc(const int* table, int nrows, int ncols, int key, int col) {
    for (int i = 0; i < nrows - 1; i++) {
        int a = table[i * ncols], b = table[(i + 1) * ncols];
        if (key <= a && key >= b) {
            double frac = static_cast<double>(a - key) / (a - b);
            return lerp(table[i * ncols + col], table[(i + 1) * ncols + col], frac);
        }
    }
    if (key >= table[0]) return table[col];
    return table[(nrows - 1) * ncols + col];
}

Paces lookup_paces(int marathon_goal_sec) {
    Paces p{};
    p.marathon_sec = marathon_goal_sec;

    p.hmp_sec      = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 1);
    p.easy_a       = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 2);
    p.easy_b       = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 3);
    p.easy_c       = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 4);
    p.moderate_long= interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 5);
    p.tempo        = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 6);
    p.strength     = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 7);
    p.k10          = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 8);
    p.k5           = interp_desc(&kT35[0][0], kT35_rows, 10, marathon_goal_sec, 9);

    // Derive 5K race time from 5K pace (sec/mile)
    int sk5_sec = static_cast<int>(std::round(p.k5 * (5000.0 / 1609.344)));

    p.r400  = interp_asc(&kR400[0][0],  kSpeed_rows, 3, sk5_sec, 2);
    p.r600  = interp_asc(&kR600[0][0],  kSpeed_rows, 3, sk5_sec, 2);
    p.r800  = interp_asc(&kR800[0][0],  kSpeed_rows, 3, sk5_sec, 2);
    p.r1k   = interp_asc(&kR1K[0][0],   kSpeed_rows, 3, sk5_sec, 2);
    p.r1200 = interp_asc(&kR1200[0][0], kSpeed_rows, 3, sk5_sec, 2);
    p.r1600 = interp_asc(&kR1600[0][0], kSpeed_rows, 3, sk5_sec, 2);

    p.s1mi   = interp_asc(&kS1MI[0][0],  kS_rows,    3, marathon_goal_sec, 2);
    p.s1_5mi = interp_asc(&kS15MI[0][0], kS_rows,    3, marathon_goal_sec, 2);
    p.s2mi   = interp_asc(&kS2MI[0][0],  kS2MI_rows, 3, marathon_goal_sec, 2);
    p.s3mi   = interp_asc(&kS3MI[0][0],  kS_rows,    3, marathon_goal_sec, 2);

    return p;
}

} // namespace hanson
