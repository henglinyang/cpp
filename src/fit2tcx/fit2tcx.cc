#include "fit_decode.hpp"
#include "fit_mesg_broadcaster.hpp"
#include "fit_developer_field_description.hpp"
#include "fit_record_mesg.hpp"
#include "fit_record_mesg_listener.hpp"
#include "fit_lap_mesg.hpp"
#include "fit_lap_mesg_listener.hpp"
#include "fit_session_mesg.hpp"
#include "fit_session_mesg_listener.hpp"
#include "pugixml.hpp"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

// FIT timestamps are seconds since 1989-12-31T00:00:00Z
static const uint32_t kFitEpoch = 631065600;

struct RecordData {
    FIT_DATE_TIME timestamp = 0;
    bool has_lat = false;
    bool has_lon = false;
    bool has_altitude = false;
    bool has_distance = false;
    bool has_speed = false;
    bool has_power = false;
    bool has_hr = false;
    bool has_cadence = false;
    double lat = 0;
    double lon = 0;
    double altitude = 0;
    double distance = 0;
    double speed = 0;
    int heart_rate = 0;
    int cadence = 0;
    int power = 0;
};

struct LapData {
    FIT_DATE_TIME start_time = 0;
    FIT_DATE_TIME timestamp = 0;
    bool has_elapsed = false;
    bool has_distance = false;
    bool has_calories = false;
    bool has_avg_hr = false;
    bool has_max_hr = false;
    bool has_avg_cadence = false;
    double total_elapsed_time = 0;
    double total_distance = 0;
    int total_calories = 0;
    int avg_heart_rate = 0;
    int max_heart_rate = 0;
    int avg_cadence = 0;
};

struct SessionData {
    FIT_DATE_TIME start_time = 0;
    FIT_SPORT sport = FIT_SPORT_INVALID;
    bool has_start_time = false;
    bool has_sport = false;
};

static std::string fitToIso8601(FIT_DATE_TIME ts) {
    time_t t = static_cast<time_t>(ts) + kFitEpoch;
    struct tm tm_info;
    gmtime_r(&t, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
    return buf;
}

static double semicirclesToDeg(FIT_SINT32 sc) {
    return sc * (180.0 / 2147483648.0);
}

static const char* sportToTcx(FIT_SPORT sport) {
    switch (sport) {
        case FIT_SPORT_RUNNING:    return "Running";
        case FIT_SPORT_CYCLING:    return "Biking";
        case FIT_SPORT_SWIMMING:   return "Swimming";
        case FIT_SPORT_WALKING:    return "Running";
        default:                   return "Other";
    }
}

class Listener
    : public fit::RecordMesgListener
    , public fit::LapMesgListener
    , public fit::SessionMesgListener
    , public fit::DeveloperFieldDescriptionListener
{
public:
    std::vector<RecordData> records;
    std::vector<LapData> laps;
    SessionData session;
    bool has_session = false;

    void OnMesg(fit::RecordMesg& mesg) override {
        if (!mesg.IsTimestampValid()) return;

        RecordData r;
        r.timestamp = mesg.GetTimestamp();

        if (mesg.IsPositionLatValid() && mesg.IsPositionLongValid()) {
            r.has_lat = r.has_lon = true;
            r.lat = semicirclesToDeg(mesg.GetPositionLat());
            r.lon = semicirclesToDeg(mesg.GetPositionLong());
        }

        if (mesg.IsEnhancedAltitudeValid()) {
            r.has_altitude = true;
            r.altitude = mesg.GetEnhancedAltitude();
        } else if (mesg.IsAltitudeValid()) {
            r.has_altitude = true;
            r.altitude = mesg.GetAltitude();
        }

        if (mesg.IsHeartRateValid()) {
            r.has_hr = true;
            r.heart_rate = mesg.GetHeartRate();
        }

        if (mesg.IsCadenceValid()) {
            r.has_cadence = true;
            r.cadence = mesg.GetCadence();
        }

        if (mesg.IsDistanceValid()) {
            r.has_distance = true;
            r.distance = mesg.GetDistance();
        }

        if (mesg.IsEnhancedSpeedValid()) {
            r.has_speed = true;
            r.speed = mesg.GetEnhancedSpeed();
        } else if (mesg.IsSpeedValid()) {
            r.has_speed = true;
            r.speed = mesg.GetSpeed();
        }

        if (mesg.IsPowerValid()) {
            r.has_power = true;
            r.power = mesg.GetPower();
        }

        records.push_back(r);
    }

    void OnMesg(fit::LapMesg& mesg) override {
        LapData l;
        l.start_time = mesg.IsStartTimeValid() ? mesg.GetStartTime() : 0;
        l.timestamp  = mesg.IsTimestampValid()  ? mesg.GetTimestamp()  : 0;

        if (mesg.IsTotalElapsedTimeValid()) {
            l.has_elapsed = true;
            l.total_elapsed_time = mesg.GetTotalElapsedTime();
        }
        if (mesg.IsTotalDistanceValid()) {
            l.has_distance = true;
            l.total_distance = mesg.GetTotalDistance();
        }
        if (mesg.IsTotalCaloriesValid()) {
            l.has_calories = true;
            l.total_calories = mesg.GetTotalCalories();
        }
        if (mesg.IsAvgHeartRateValid()) {
            l.has_avg_hr = true;
            l.avg_heart_rate = mesg.GetAvgHeartRate();
        }
        if (mesg.IsMaxHeartRateValid()) {
            l.has_max_hr = true;
            l.max_heart_rate = mesg.GetMaxHeartRate();
        }
        if (mesg.IsAvgCadenceValid()) {
            l.has_avg_cadence = true;
            l.avg_cadence = mesg.GetAvgCadence();
        }

        laps.push_back(l);
    }

    void OnMesg(fit::SessionMesg& mesg) override {
        has_session = true;
        if (mesg.IsStartTimeValid()) {
            session.has_start_time = true;
            session.start_time = mesg.GetStartTime();
        }
        if (mesg.IsSportValid()) {
            session.has_sport = true;
            session.sport = mesg.GetSport();
        }
    }

    void OnDeveloperFieldDescription(const fit::DeveloperFieldDescription&) override {}
};

static void addHrNode(pugi::xml_node parent, const char* tag, int bpm) {
    parent.append_child(tag).append_child("Value").text().set(bpm);
}

static void writeTcx(const std::vector<RecordData>& records,
                     const std::vector<LapData>& laps,
                     const SessionData& session,
                     bool has_session)
{
    pugi::xml_document doc;

    auto decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version")  = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto root = doc.append_child("TrainingCenterDatabase");
    root.append_attribute("xmlns") =
        "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2";
    root.append_attribute("xmlns:xsi") =
        "http://www.w3.org/2001/XMLSchema-instance";
    root.append_attribute("xsi:schemaLocation") =
        "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 "
        "http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd";

    auto activities = root.append_child("Activities");
    auto activity   = activities.append_child("Activity");

    const char* sport = "Other";
    if (has_session && session.has_sport)
        sport = sportToTcx(session.sport);
    activity.append_attribute("Sport") = sport;

    FIT_DATE_TIME activityStart = 0;
    if (has_session && session.has_start_time)
        activityStart = session.start_time;
    else if (!records.empty())
        activityStart = records.front().timestamp;
    activity.append_child("Id").text().set(fitToIso8601(activityStart).c_str());

    // If no lap messages, synthesize one lap covering all records.
    std::vector<LapData> effectiveLaps = laps;
    if (effectiveLaps.empty() && !records.empty()) {
        LapData synth;
        synth.start_time = records.front().timestamp;
        synth.timestamp  = records.back().timestamp;
        if (synth.timestamp > synth.start_time) {
            synth.has_elapsed = true;
            synth.total_elapsed_time =
                static_cast<double>(synth.timestamp - synth.start_time);
        }
        if (records.back().has_distance) {
            synth.has_distance = true;
            synth.total_distance = records.back().distance;
        }
        effectiveLaps.push_back(synth);
    }

    for (const auto& lap : effectiveLaps) {
        auto lapNode = activity.append_child("Lap");
        lapNode.append_attribute("StartTime") =
            fitToIso8601(lap.start_time).c_str();

        if (lap.has_elapsed)
            lapNode.append_child("TotalTimeSeconds").text().set(lap.total_elapsed_time);
        if (lap.has_distance)
            lapNode.append_child("DistanceMeters").text().set(lap.total_distance);
        if (lap.has_calories)
            lapNode.append_child("Calories").text().set(lap.total_calories);
        if (lap.has_avg_hr)
            addHrNode(lapNode, "AverageHeartRateBpm", lap.avg_heart_rate);
        if (lap.has_max_hr)
            addHrNode(lapNode, "MaximumHeartRateBpm", lap.max_heart_rate);
        lapNode.append_child("Intensity").text().set("Active");
        lapNode.append_child("TriggerMethod").text().set("Manual");

        auto track = lapNode.append_child("Track");
        for (const auto& rec : records) {
            if (rec.timestamp < lap.start_time) continue;
            if (lap.timestamp > 0 && rec.timestamp > lap.timestamp) continue;

            auto tp = track.append_child("Trackpoint");
            tp.append_child("Time").text().set(fitToIso8601(rec.timestamp).c_str());

            if (rec.has_lat && rec.has_lon) {
                auto pos = tp.append_child("Position");
                std::ostringstream lat_ss, lon_ss;
                lat_ss << std::fixed << std::setprecision(7) << rec.lat;
                lon_ss << std::fixed << std::setprecision(7) << rec.lon;
                pos.append_child("LatitudeDegrees").text().set(lat_ss.str().c_str());
                pos.append_child("LongitudeDegrees").text().set(lon_ss.str().c_str());
            }

            if (rec.has_altitude)
                tp.append_child("AltitudeMeters").text().set(rec.altitude);
            if (rec.has_distance)
                tp.append_child("DistanceMeters").text().set(rec.distance);
            if (rec.has_hr)
                addHrNode(tp, "HeartRateBpm", rec.heart_rate);
            if (rec.has_cadence)
                tp.append_child("Cadence").text().set(rec.cadence);

            if (rec.has_speed || rec.has_power) {
                auto tpx = tp.append_child("Extensions").append_child("TPX");
                tpx.append_attribute("xmlns") =
                    "http://www.garmin.com/xmlschemas/ActivityExtension/v2";
                if (rec.has_speed)
                    tpx.append_child("Speed").text().set(rec.speed);
                if (rec.has_power)
                    tpx.append_child("Watts").text().set(rec.power);
            }
        }
    }

    doc.save(std::cout, "  ");
}

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

    fit::Decode decode;
    fit::MesgBroadcaster mesgBroadcaster;
    Listener listener;

    if (!decode.CheckIntegrity(file))
        fprintf(stderr, "FIT integrity check failed, attempting decode anyway...\n");

    mesgBroadcaster.AddListener(static_cast<fit::RecordMesgListener&>(listener));
    mesgBroadcaster.AddListener(static_cast<fit::LapMesgListener&>(listener));
    mesgBroadcaster.AddListener(static_cast<fit::SessionMesgListener&>(listener));

    try {
        decode.Read(&file, &mesgBroadcaster, &mesgBroadcaster, &listener);
    } catch (const fit::RuntimeException& e) {
        fprintf(stderr, "Exception decoding file: %s\n", e.what());
        return 1;
    } catch (...) {
        fprintf(stderr, "Unknown exception decoding file\n");
        return 1;
    }

    writeTcx(listener.records, listener.laps, listener.session, listener.has_session);
    return 0;
}
