#include "fit2tcx.h"
#include "fit_profile.hpp"
#include "pugixml.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---- minimal test harness ----

struct TestCase { std::string name; std::function<void()> fn; };
static std::vector<TestCase>& tests() {
    static std::vector<TestCase> v;
    return v;
}
struct RegisterTest {
    RegisterTest(const char* n, std::function<void()> f) { tests().push_back({n, f}); }
};
#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static RegisterTest reg_##suite##_##name(#suite "." #name, test_##suite##_##name); \
    static void test_##suite##_##name()

static void check(bool cond, const std::string& msg, const char* file, int line) {
    if (!cond) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + msg);
}
#define EXPECT_TRUE(x)         check((x),          #x " is false",                __FILE__, __LINE__)
#define EXPECT_FALSE(x)        check(!(x),          #x " is true",                 __FILE__, __LINE__)
#define EXPECT_EQ(a, b)        check((a) == (b),   #a " != " #b,                  __FILE__, __LINE__)
#define EXPECT_STREQ(a, b)     check(std::string(a) == std::string(b), \
                                     std::string(#a) + " != " + std::string(#b),  __FILE__, __LINE__)
#define EXPECT_NEAR(a, b, tol) check(std::abs(static_cast<double>(a) - static_cast<double>(b)) <= (tol), \
                                     std::string(#a) + " not near " + #b,         __FILE__, __LINE__)

// ---- helpers ----

static pugi::xml_document parseTcx(const std::string& xml) {
    pugi::xml_document doc;
    auto result = doc.load_string(xml.c_str());
    check(result, "XML parse failed", __FILE__, __LINE__);
    return doc;
}

static int countChildren(pugi::xml_node parent, const char* name) {
    int n = 0;
    for (auto c : parent.children(name)) { (void)c; n++; }
    return n;
}

static std::string runWriteTcx(const std::vector<RecordData>& records,
                                const std::vector<LapData>&   laps,
                                const SessionData&            session,
                                bool has_session) {
    std::ostringstream out;
    writeTcx(records, laps, session, has_session, out);
    return out.str();
}

// ---- fitToIso8601 ----

TEST(FitToIso8601, FitEpoch) {
    EXPECT_EQ(fitToIso8601(0), "1989-12-31T00:00:00Z");
}
TEST(FitToIso8601, OneDay) {
    EXPECT_EQ(fitToIso8601(86400), "1990-01-01T00:00:00Z");
}
TEST(FitToIso8601, Year2020) {
    // Unix 1577836800 = 2020-01-01T00:00:00Z → FIT = 1577836800 - 631065600 = 946771200
    EXPECT_EQ(fitToIso8601(946771200), "2020-01-01T00:00:00Z");
}
TEST(FitToIso8601, EndOfDay) {
    EXPECT_EQ(fitToIso8601(86399), "1989-12-31T23:59:59Z");
}

// ---- semicirclesToDeg ----

TEST(SemicirclesToDeg, Zero) {
    EXPECT_NEAR(semicirclesToDeg(0), 0.0, 1e-9);
}
TEST(SemicirclesToDeg, Ninety) {
    // 2^31 / 2 = 1073741824 semicircles = 90°
    EXPECT_NEAR(semicirclesToDeg(1073741824), 90.0, 1e-6);
}
TEST(SemicirclesToDeg, NegativeHundredEighty) {
    EXPECT_NEAR(semicirclesToDeg(-2147483648), -180.0, 1e-6);
}
TEST(SemicirclesToDeg, RoundTrip) {
    double lat = 51.5074;
    int32_t sc = static_cast<int32_t>(lat * 2147483648.0 / 180.0);
    EXPECT_NEAR(semicirclesToDeg(sc), lat, 1e-4);
}

// ---- sportToTcx ----

TEST(SportToTcx, Running)  { EXPECT_STREQ(sportToTcx(FIT_SPORT_RUNNING),  "Running");  }
TEST(SportToTcx, Cycling)  { EXPECT_STREQ(sportToTcx(FIT_SPORT_CYCLING),  "Biking");   }
TEST(SportToTcx, Swimming) { EXPECT_STREQ(sportToTcx(FIT_SPORT_SWIMMING), "Swimming"); }
TEST(SportToTcx, Walking)  { EXPECT_STREQ(sportToTcx(FIT_SPORT_WALKING),  "Running");  }
TEST(SportToTcx, Generic)  { EXPECT_STREQ(sportToTcx(FIT_SPORT_GENERIC),  "Other");    }
TEST(SportToTcx, Unknown)  { EXPECT_STREQ(sportToTcx(255),                "Other");    }

// ---- writeTcx: document structure ----

TEST(WriteTcx, RootNamespace) {
    auto doc = parseTcx(runWriteTcx({}, {}, SessionData{}, false));
    auto root = doc.child("TrainingCenterDatabase");
    EXPECT_TRUE(root);
    EXPECT_STREQ(root.attribute("xmlns").value(),
        "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2");
}
TEST(WriteTcx, HasActivityNode) {
    auto doc = parseTcx(runWriteTcx({}, {}, SessionData{}, false));
    EXPECT_TRUE(doc.child("TrainingCenterDatabase").child("Activities").child("Activity"));
}
TEST(WriteTcx, DefaultSportOther) {
    auto doc = parseTcx(runWriteTcx({}, {}, SessionData{}, false));
    auto activity = doc.child("TrainingCenterDatabase").child("Activities").child("Activity");
    EXPECT_STREQ(activity.attribute("Sport").value(), "Other");
}
TEST(WriteTcx, ActivitySportFromSession) {
    SessionData s; s.has_sport = true; s.sport = FIT_SPORT_CYCLING;
    auto doc = parseTcx(runWriteTcx({}, {}, s, true));
    auto activity = doc.child("TrainingCenterDatabase").child("Activities").child("Activity");
    EXPECT_STREQ(activity.attribute("Sport").value(), "Biking");
}
TEST(WriteTcx, ActivityIdFromSession) {
    SessionData s; s.has_start_time = true; s.start_time = 946771200;
    auto doc = parseTcx(runWriteTcx({}, {}, s, true));
    auto id = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Id");
    EXPECT_STREQ(id.text().as_string(), "2020-01-01T00:00:00Z");
}
TEST(WriteTcx, ActivityIdFallsBackToFirstRecord) {
    RecordData r; r.timestamp = 946771200;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto id = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Id");
    EXPECT_STREQ(id.text().as_string(), "2020-01-01T00:00:00Z");
}

// ---- writeTcx: synthesized lap ----

TEST(WriteTcx, SynthesizedLapElapsedTime) {
    RecordData r1, r2;
    r1.timestamp = 946771200;
    r2.timestamp = 946771260;
    auto doc = parseTcx(runWriteTcx({r1, r2}, {}, SessionData{}, false));
    auto lap = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Lap");
    EXPECT_TRUE(lap);
    EXPECT_NEAR(lap.child("TotalTimeSeconds").text().as_double(), 60.0, 0.01);
}
TEST(WriteTcx, SynthesizedLapTrackpointCount) {
    RecordData r1, r2, r3;
    r1.timestamp = 946771200; r2.timestamp = 946771210; r3.timestamp = 946771220;
    auto doc = parseTcx(runWriteTcx({r1, r2, r3}, {}, SessionData{}, false));
    auto lap = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Lap");
    EXPECT_EQ(countChildren(lap.child("Track"), "Trackpoint"), 3);
}
TEST(WriteTcx, SynthesizedLapDistanceFromLastRecord) {
    RecordData r1, r2;
    r1.timestamp = 946771200;
    r2.timestamp = 946771300; r2.has_distance = true; r2.distance = 500.0;
    auto doc = parseTcx(runWriteTcx({r1, r2}, {}, SessionData{}, false));
    auto lap = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Lap");
    EXPECT_NEAR(lap.child("DistanceMeters").text().as_double(), 500.0, 0.01);
}

// ---- writeTcx: trackpoint fields ----

TEST(WriteTcx, TrackpointTime) {
    RecordData r; r.timestamp = 946771200;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tp = doc.child("TrainingCenterDatabase").child("Activities")
                 .child("Activity").child("Lap").child("Track").child("Trackpoint");
    EXPECT_STREQ(tp.child("Time").text().as_string(), "2020-01-01T00:00:00Z");
}
TEST(WriteTcx, TrackpointPosition) {
    RecordData r; r.timestamp = 946771200;
    r.has_lat = r.has_lon = true; r.lat = 37.7749; r.lon = -122.4194;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto pos = doc.child("TrainingCenterDatabase").child("Activities")
                  .child("Activity").child("Lap").child("Track")
                  .child("Trackpoint").child("Position");
    EXPECT_TRUE(pos);
    EXPECT_NEAR(pos.child("LatitudeDegrees").text().as_double(),   37.7749,   1e-4);
    EXPECT_NEAR(pos.child("LongitudeDegrees").text().as_double(), -122.4194,  1e-4);
}
TEST(WriteTcx, TrackpointNoPositionWhenMissing) {
    RecordData r; r.timestamp = 946771200;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tp = doc.child("TrainingCenterDatabase").child("Activities")
                 .child("Activity").child("Lap").child("Track").child("Trackpoint");
    EXPECT_FALSE(tp.child("Position"));
}
TEST(WriteTcx, TrackpointAltitudeHrCadence) {
    RecordData r; r.timestamp = 946771200;
    r.has_altitude = true; r.altitude = 50.0;
    r.has_hr = true;       r.heart_rate = 150;
    r.has_cadence = true;  r.cadence = 80;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tp = doc.child("TrainingCenterDatabase").child("Activities")
                 .child("Activity").child("Lap").child("Track").child("Trackpoint");
    EXPECT_NEAR(tp.child("AltitudeMeters").text().as_double(), 50.0, 0.01);
    EXPECT_EQ(tp.child("HeartRateBpm").child("Value").text().as_int(), 150);
    EXPECT_EQ(tp.child("Cadence").text().as_int(), 80);
}
TEST(WriteTcx, TrackpointSpeedExtension) {
    RecordData r; r.timestamp = 946771200; r.has_speed = true; r.speed = 3.5;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tpx = doc.child("TrainingCenterDatabase").child("Activities")
                  .child("Activity").child("Lap").child("Track")
                  .child("Trackpoint").child("Extensions").child("TPX");
    EXPECT_TRUE(tpx);
    EXPECT_NEAR(tpx.child("Speed").text().as_double(), 3.5, 0.001);
}
TEST(WriteTcx, TrackpointPowerExtension) {
    RecordData r; r.timestamp = 946771200; r.has_power = true; r.power = 200;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tpx = doc.child("TrainingCenterDatabase").child("Activities")
                  .child("Activity").child("Lap").child("Track")
                  .child("Trackpoint").child("Extensions").child("TPX");
    EXPECT_TRUE(tpx);
    EXPECT_EQ(tpx.child("Watts").text().as_int(), 200);
}
TEST(WriteTcx, NoExtensionWhenNoSpeedOrPower) {
    RecordData r; r.timestamp = 946771200; r.has_hr = true; r.heart_rate = 140;
    auto doc = parseTcx(runWriteTcx({r}, {}, SessionData{}, false));
    auto tp = doc.child("TrainingCenterDatabase").child("Activities")
                 .child("Activity").child("Lap").child("Track").child("Trackpoint");
    EXPECT_FALSE(tp.child("Extensions"));
}

// ---- writeTcx: explicit lap stats ----

TEST(WriteTcx, LapStats) {
    LapData l;
    l.start_time = 946771200; l.timestamp = 946771800;
    l.has_elapsed  = true; l.total_elapsed_time = 600.0;
    l.has_distance = true; l.total_distance     = 1500.0;
    l.has_calories = true; l.total_calories     = 80;
    l.has_avg_hr   = true; l.avg_heart_rate     = 155;
    l.has_max_hr   = true; l.max_heart_rate     = 170;
    auto doc = parseTcx(runWriteTcx({}, {l}, SessionData{}, false));
    auto lap = doc.child("TrainingCenterDatabase").child("Activities").child("Activity").child("Lap");
    EXPECT_TRUE(lap);
    EXPECT_NEAR(lap.child("TotalTimeSeconds").text().as_double(), 600.0,  0.01);
    EXPECT_NEAR(lap.child("DistanceMeters").text().as_double(),   1500.0, 0.01);
    EXPECT_EQ(lap.child("Calories").text().as_int(), 80);
    EXPECT_EQ(lap.child("AverageHeartRateBpm").child("Value").text().as_int(), 155);
    EXPECT_EQ(lap.child("MaximumHeartRateBpm").child("Value").text().as_int(), 170);
    EXPECT_STREQ(lap.child("Intensity").text().as_string(),     "Active");
    EXPECT_STREQ(lap.child("TriggerMethod").text().as_string(), "Manual");
}

// ---- writeTcx: record-to-lap assignment ----

TEST(WriteTcx, RecordsGroupedByLap) {
    LapData l1, l2;
    l1.start_time = 946771200; l1.timestamp = 946771500;
    l2.start_time = 946771500; l2.timestamp = 946771800;
    RecordData r1, r2, r3;
    r1.timestamp = 946771250;
    r2.timestamp = 946771450;
    r3.timestamp = 946771600;
    auto doc = parseTcx(runWriteTcx({r1, r2, r3}, {l1, l2}, SessionData{}, false));
    auto activity = doc.child("TrainingCenterDatabase").child("Activities").child("Activity");
    auto it = activity.children("Lap").begin();
    auto firstLap = *it++;
    auto secondLap = *it;
    EXPECT_EQ(countChildren(firstLap.child("Track"),  "Trackpoint"), 2);
    EXPECT_EQ(countChildren(secondLap.child("Track"), "Trackpoint"), 1);
}
TEST(WriteTcx, MultiLapCount) {
    LapData l1, l2, l3;
    l1.start_time = 946771200; l1.timestamp = 946771400;
    l2.start_time = 946771400; l2.timestamp = 946771600;
    l3.start_time = 946771600; l3.timestamp = 946771800;
    auto doc = parseTcx(runWriteTcx({}, {l1, l2, l3}, SessionData{}, false));
    auto activity = doc.child("TrainingCenterDatabase").child("Activities").child("Activity");
    EXPECT_EQ(countChildren(activity, "Lap"), 3);
}

// ---- main ----

int main() {
    int passed = 0, failed = 0;
    for (const auto& t : tests()) {
        try {
            t.fn();
            printf("[ OK ] %s\n", t.name.c_str());
            passed++;
        } catch (const std::exception& e) {
            printf("[FAIL] %s  —  %s\n", t.name.c_str(), e.what());
            failed++;
        }
    }
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
