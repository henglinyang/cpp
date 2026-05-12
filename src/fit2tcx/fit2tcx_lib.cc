#include "fit2tcx.h"

#include "fit_decode.hpp"
#include "fit_mesg_broadcaster.hpp"
#include "fit_developer_field_description.hpp"
#include "fit_record_mesg.hpp"
#include "fit_record_mesg_listener.hpp"
#include "fit_lap_mesg.hpp"
#include "fit_lap_mesg_listener.hpp"
#include "fit_session_mesg.hpp"
#include "fit_session_mesg_listener.hpp"
#include "fit_profile.hpp"
#include "pugixml.hpp"

#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <ctime>

// FIT timestamps are seconds since 1989-12-31T00:00:00Z.
static const uint32_t kFitEpoch = 631065600;

std::string fitToIso8601(uint32_t ts) {
    time_t t = static_cast<time_t>(ts) + kFitEpoch;
    struct tm tm_info;
    gmtime_r(&t, &tm_info);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);
    return buf;
}

double semicirclesToDeg(int32_t sc) {
    return sc * (180.0 / 2147483648.0);
}

const char* sportToTcx(uint8_t sport) {
    switch (sport) {
        case FIT_SPORT_RUNNING:  return "Running";
        case FIT_SPORT_CYCLING:  return "Biking";
        case FIT_SPORT_SWIMMING: return "Swimming";
        case FIT_SPORT_WALKING:  return "Running";
        default:                 return "Other";
    }
}

// ---- FIT listener ----

class Listener
    : public fit::RecordMesgListener
    , public fit::LapMesgListener
    , public fit::SessionMesgListener
    , public fit::DeveloperFieldDescriptionListener
{
public:
    FitData data;

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
        data.records.push_back(r);
    }

    void OnMesg(fit::LapMesg& mesg) override {
        LapData l;
        l.start_time = mesg.IsStartTimeValid() ? mesg.GetStartTime() : 0;
        l.timestamp  = mesg.IsTimestampValid()  ? mesg.GetTimestamp()  : 0;
        if (mesg.IsTotalElapsedTimeValid()) { l.has_elapsed  = true; l.total_elapsed_time = mesg.GetTotalElapsedTime(); }
        if (mesg.IsTotalDistanceValid())    { l.has_distance = true; l.total_distance     = mesg.GetTotalDistance(); }
        if (mesg.IsTotalCaloriesValid())    { l.has_calories = true; l.total_calories     = mesg.GetTotalCalories(); }
        if (mesg.IsAvgHeartRateValid())     { l.has_avg_hr   = true; l.avg_heart_rate     = mesg.GetAvgHeartRate(); }
        if (mesg.IsMaxHeartRateValid())     { l.has_max_hr   = true; l.max_heart_rate     = mesg.GetMaxHeartRate(); }
        if (mesg.IsAvgCadenceValid())       { l.has_avg_cadence = true; l.avg_cadence     = mesg.GetAvgCadence(); }
        data.laps.push_back(l);
    }

    void OnMesg(fit::SessionMesg& mesg) override {
        data.has_session = true;
        if (mesg.IsStartTimeValid()) { data.session.has_start_time = true; data.session.start_time = mesg.GetStartTime(); }
        if (mesg.IsSportValid())     { data.session.has_sport      = true; data.session.sport      = mesg.GetSport(); }
    }

    void OnDeveloperFieldDescription(const fit::DeveloperFieldDescription&) override {}
};

// ---- TCX writer ----

static void addHrNode(pugi::xml_node parent, const char* tag, int bpm) {
    parent.append_child(tag).append_child("Value").text().set(bpm);
}

void writeTcx(const std::vector<RecordData>& records,
              const std::vector<LapData>& laps,
              const SessionData& session,
              bool has_session,
              std::ostream& out)
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

    auto activity = root.append_child("Activities").append_child("Activity");
    activity.append_attribute("Sport") =
        (has_session && session.has_sport) ? sportToTcx(session.sport) : "Other";

    uint32_t activityStart = 0;
    if (has_session && session.has_start_time)
        activityStart = session.start_time;
    else if (!records.empty())
        activityStart = records.front().timestamp;
    activity.append_child("Id").text().set(fitToIso8601(activityStart).c_str());

    std::vector<LapData> effectiveLaps = laps;
    if (effectiveLaps.empty() && !records.empty()) {
        LapData synth;
        synth.start_time = records.front().timestamp;
        synth.timestamp  = records.back().timestamp;
        if (synth.timestamp > synth.start_time) {
            synth.has_elapsed = true;
            synth.total_elapsed_time = static_cast<double>(synth.timestamp - synth.start_time);
        }
        if (records.back().has_distance) {
            synth.has_distance = true;
            synth.total_distance = records.back().distance;
        }
        effectiveLaps.push_back(synth);
    }

    for (const auto& lap : effectiveLaps) {
        auto lapNode = activity.append_child("Lap");
        lapNode.append_attribute("StartTime") = fitToIso8601(lap.start_time).c_str();

        if (lap.has_elapsed)  lapNode.append_child("TotalTimeSeconds").text().set(lap.total_elapsed_time);
        if (lap.has_distance) lapNode.append_child("DistanceMeters").text().set(lap.total_distance);
        if (lap.has_calories) lapNode.append_child("Calories").text().set(lap.total_calories);
        if (lap.has_avg_hr)   addHrNode(lapNode, "AverageHeartRateBpm",  lap.avg_heart_rate);
        if (lap.has_max_hr)   addHrNode(lapNode, "MaximumHeartRateBpm",  lap.max_heart_rate);
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
            if (rec.has_altitude) tp.append_child("AltitudeMeters").text().set(rec.altitude);
            if (rec.has_distance) tp.append_child("DistanceMeters").text().set(rec.distance);
            if (rec.has_hr)       addHrNode(tp, "HeartRateBpm", rec.heart_rate);
            if (rec.has_cadence)  tp.append_child("Cadence").text().set(rec.cadence);

            if (rec.has_speed || rec.has_power) {
                auto tpx = tp.append_child("Extensions").append_child("TPX");
                tpx.append_attribute("xmlns") =
                    "http://www.garmin.com/xmlschemas/ActivityExtension/v2";
                if (rec.has_speed) tpx.append_child("Speed").text().set(rec.speed);
                if (rec.has_power) tpx.append_child("Watts").text().set(rec.power);
            }
        }
    }

    doc.save(out, "  ");
}

// ---- FIT decoder ----

FitData decodeFit(std::istream& file) {
    fit::Decode decode;
    fit::MesgBroadcaster broadcaster;
    Listener listener;

    if (!decode.CheckIntegrity(file))
        fprintf(stderr, "FIT integrity check failed, attempting decode anyway...\n");

    broadcaster.AddListener(static_cast<fit::RecordMesgListener&>(listener));
    broadcaster.AddListener(static_cast<fit::LapMesgListener&>(listener));
    broadcaster.AddListener(static_cast<fit::SessionMesgListener&>(listener));

    try {
        decode.Read(&file, &broadcaster, &broadcaster, &listener);
    } catch (const fit::RuntimeException& e) {
        throw std::runtime_error(std::string("FIT decode error: ") + e.what());
    }

    return listener.data;
}
