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
#include "fit_workout_mesg.hpp"
#include "fit_workout_mesg_listener.hpp"
#include "fit_workout_step_mesg.hpp"
#include "fit_workout_step_mesg_listener.hpp"
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

// ---- helpers ----

static std::string wstringToString(const FIT_WSTRING& ws) {
    std::string s;
    s.reserve(ws.size());
    for (wchar_t c : ws) s += static_cast<char>(c);
    return s;
}

// ---- FIT listener ----

class Listener
    : public fit::RecordMesgListener
    , public fit::LapMesgListener
    , public fit::SessionMesgListener
    , public fit::WorkoutMesgListener
    , public fit::WorkoutStepMesgListener
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

    void OnMesg(fit::WorkoutMesg& mesg) override {
        data.has_workout = true;
        if (mesg.IsSportValid())   { data.workout.has_sport = true; data.workout.sport = mesg.GetSport(); }
        if (mesg.IsWktNameValid()) { data.workout.has_name  = true; data.workout.name  = wstringToString(mesg.GetWktName()); }
    }

    void OnMesg(fit::WorkoutStepMesg& mesg) override {
        WorkoutStepData s;
        if (mesg.IsMessageIndexValid()) s.step_index     = mesg.GetMessageIndex();
        if (mesg.IsDurationTypeValid()) s.duration_type  = mesg.GetDurationType();
        if (mesg.IsDurationValueValid()) s.duration_value = mesg.GetDurationValue();
        if (mesg.IsTargetTypeValid())   s.target_type    = mesg.GetTargetType();
        if (mesg.IsTargetValueValid())  s.target_value   = mesg.GetTargetValue();
        if (mesg.IsIntensityValid())    s.intensity      = mesg.GetIntensity();
        if (mesg.IsWktStepNameValid()) { s.has_name = true; s.name = wstringToString(mesg.GetWktStepName()); }
        data.workout.steps.push_back(s);
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

static const char* intensityToTcx(uint8_t intensity) {
    // TCX Intensity_t only permits "Active" or "Resting"
    return (intensity == FIT_INTENSITY_REST) ? "Resting" : "Active";
}

// ---- Garmin Connect workout JSON writer ----

static std::string jsonStr(const std::string& s) {
    std::string r = "\"";
    for (unsigned char c : s) {
        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "\\r";
        else if (c == '\t') r += "\\t";
        else r += static_cast<char>(c);
    }
    return r + "\"";
}

static std::string jsonDbl(double v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.7g", v);
    std::string s = buf;
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
        s += ".0";
    return s;
}

static void writeExecStep(std::ostream& out, const WorkoutStepData& step,
                           int& ord, bool inRepeat) {
    const char* stepKey; int stepTypeId;
    switch (step.intensity) {
        case FIT_INTENSITY_WARMUP:   stepKey = "warmup";   stepTypeId = 1; break;
        case FIT_INTENSITY_COOLDOWN: stepKey = "cooldown"; stepTypeId = 2; break;
        case FIT_INTENSITY_REST:     stepKey = "recovery"; stepTypeId = 4; break;
        default:                     stepKey = "interval"; stepTypeId = 3; break;
    }

    out << "{\"type\":\"ExecutableStepDTO\","
        << "\"stepId\":0,"
        << "\"stepOrder\":" << ord++ << ","
        << "\"stepType\":{\"stepTypeId\":" << stepTypeId
        <<   ",\"stepTypeKey\":\"" << stepKey
        <<   "\",\"displayOrder\":" << stepTypeId << "},"
        << "\"childStepId\":" << (inRepeat ? "1" : "null") << ",";

    out << "\"description\":"
        << (step.has_name && !step.name.empty() ? jsonStr(step.name) : "null") << ",";

    if (step.duration_type == FIT_WKT_STEP_DURATION_TIME) {
        out << "\"endCondition\":{\"conditionTypeId\":2,\"conditionTypeKey\":\"time\","
            <<   "\"displayOrder\":2,\"displayable\":true},"
            << "\"endConditionValue\":" << jsonDbl(step.duration_value / 1000.0) << ",";
    } else if (step.duration_type == FIT_WKT_STEP_DURATION_OPEN) {
        out << "\"endCondition\":{\"conditionTypeId\":1,\"conditionTypeKey\":\"lap.button\","
            <<   "\"displayOrder\":1,\"displayable\":true},"
            << "\"endConditionValue\":null,";
    } else {
        out << "\"endCondition\":{\"conditionTypeId\":3,\"conditionTypeKey\":\"distance\","
            <<   "\"displayOrder\":3,\"displayable\":true},"
            << "\"endConditionValue\":" << jsonDbl(step.duration_value / 100.0) << ",";
    }

    out << "\"preferredEndConditionUnit\":null,\"endConditionCompare\":null,";

    if (step.target_type == FIT_WKT_STEP_TARGET_SPEED &&
        (step.has_target_low || step.has_target_high)) {
        out << "\"targetType\":{\"workoutTargetTypeId\":6,"
            <<   "\"workoutTargetTypeKey\":\"pace.zone\",\"displayOrder\":6},"
            << "\"targetValueOne\":" << jsonDbl(step.target_high / 1000.0) << ","
            << "\"targetValueTwo\":" << jsonDbl(step.target_low  / 1000.0) << ",";
    } else if (step.target_type == FIT_WKT_STEP_TARGET_HEART_RATE &&
               (step.has_target_low || step.has_target_high)) {
        out << "\"targetType\":{\"workoutTargetTypeId\":4,"
            <<   "\"workoutTargetTypeKey\":\"heart.rate.zone\",\"displayOrder\":4},"
            << "\"targetValueOne\":" << jsonDbl(step.has_target_high ? (double)step.target_high : 220.0) << ","
            << "\"targetValueTwo\":" << jsonDbl(step.has_target_low  ? (double)step.target_low  : 0.0)   << ",";
    } else {
        out << "\"targetType\":{\"workoutTargetTypeId\":1,"
            <<   "\"workoutTargetTypeKey\":\"no.target\",\"displayOrder\":1},"
            << "\"targetValueOne\":null,\"targetValueTwo\":null,";
    }

    out << "\"targetValueUnit\":null,\"zoneNumber\":null,"
        << "\"secondaryTargetType\":null,\"secondaryTargetValueOne\":null,"
        << "\"secondaryTargetValueTwo\":null,\"secondaryTargetValueUnit\":null,"
        << "\"secondaryZoneNumber\":null,\"endConditionZone\":null,"
        << "\"strokeType\":{\"strokeTypeId\":0,\"strokeTypeKey\":null,\"displayOrder\":0},"
        << "\"equipmentType\":{\"equipmentTypeId\":0,\"equipmentTypeKey\":null,\"displayOrder\":0},"
        << "\"category\":null,\"exerciseName\":null,\"workoutProvider\":null,"
        << "\"providerExerciseSourceId\":null,\"weightValue\":null,\"weightUnit\":null}";
}

static void writeRepeatStep(std::ostream& out, const WorkoutStepData& step, int& ord) {
    int repeatOrd = ord++;
    out << "{\"type\":\"RepeatGroupDTO\","
        << "\"stepId\":0,"
        << "\"stepOrder\":" << repeatOrd << ","
        << "\"stepType\":{\"stepTypeId\":6,\"stepTypeKey\":\"repeat\",\"displayOrder\":6},"
        << "\"childStepId\":1,"
        << "\"numberOfIterations\":" << step.repetitions << ","
        << "\"workoutSteps\":[";
    for (size_t i = 0; i < step.children.size(); i++) {
        if (i > 0) out << ",";
        writeExecStep(out, step.children[i], ord, true);
    }
    out << "],"
        << "\"endConditionValue\":" << jsonDbl(static_cast<double>(step.repetitions)) << ","
        << "\"preferredEndConditionUnit\":null,\"endConditionCompare\":null,"
        << "\"endCondition\":{\"conditionTypeId\":7,\"conditionTypeKey\":\"iterations\","
        <<   "\"displayOrder\":7,\"displayable\":false},"
        << "\"skipLastRestStep\":false,\"smartRepeat\":false}";
}

void writeWorkoutJson(const WorkoutData& workout, std::ostream& out) {
    const char* sportKey = "other";
    int sportTypeId = 5;
    if (workout.has_sport) {
        switch (workout.sport) {
            case FIT_SPORT_RUNNING:  sportKey = "running"; sportTypeId = 1; break;
            case FIT_SPORT_CYCLING:  sportKey = "cycling"; sportTypeId = 2; break;
            case FIT_SPORT_SWIMMING: sportKey = "swimming"; sportTypeId = 5; break;
            default: break;
        }
    }
    std::string sportObj = std::string("{\"sportTypeId\":") + std::to_string(sportTypeId)
        + ",\"sportTypeKey\":\"" + sportKey + "\",\"displayOrder\":1}";

    // Build the step list (stepOrder counter is global across all steps + children).
    std::ostringstream stepsOut;
    int ord = 1;
    for (size_t i = 0; i < workout.steps.size(); i++) {
        if (i > 0) stepsOut << ",";
        const auto& s = workout.steps[i];
        if (s.is_repeat) writeRepeatStep(stepsOut, s, ord);
        else             writeExecStep(stepsOut, s, ord, false);
    }

    const std::string name = workout.has_name ? workout.name : "";
    out << "{\"workoutId\":0,\"ownerId\":0,\"workoutName\":" << jsonStr(name)
        << ",\"description\":\"\",\"updatedDate\":null,\"createdDate\":null"
        << ",\"sportType\":" << sportObj
        << ",\"subSportType\":null,\"trainingPlanId\":null,\"author\":null"
        << ",\"sharedWithUsers\":null,\"estimatedDurationInSecs\":null"
        << ",\"estimatedDistanceInMeters\":null"
        << ",\"workoutSegments\":[{\"segmentOrder\":1"
        << ",\"sportType\":" << sportObj
        << ",\"poolLengthUnit\":null,\"poolLength\":null,\"avgTrainingSpeed\":null"
        << ",\"estimatedDurationInSecs\":null,\"estimatedDistanceInMeters\":null"
        << ",\"estimatedDistanceUnit\":null,\"estimateType\":null,\"description\":null"
        << ",\"workoutSteps\":[" << stepsOut.str() << "]}]"
        << ",\"poolLength\":null,\"poolLengthUnit\":null,\"locale\":null"
        << ",\"workoutProvider\":null,\"workoutSourceId\":null,\"uploadTimestamp\":null"
        << ",\"atpPlanId\":null,\"consumer\":null,\"consumerName\":null"
        << ",\"consumerImageURL\":null,\"consumerWebsiteURL\":null"
        << ",\"workoutNameI18nKey\":null,\"descriptionI18nKey\":null"
        << ",\"avgTrainingSpeed\":null,\"estimateType\":null"
        << ",\"estimatedDistanceUnit\":null,\"workoutThumbnailUrl\":null"
        << ",\"isSessionTransitionEnabled\":null,\"shared\":false}";
}

// ---- TCX writer ----

// Write the content of one Step_t node (StepId, Name, Duration, Intensity, Target).
static void writeStepContent(pugi::xml_node node, const WorkoutStepData& step) {
    node.append_child("StepId").text().set(static_cast<int>(step.step_index) + 1);

    if (step.has_name && !step.name.empty())
        node.append_child("Name").text().set(step.name.c_str());

    if (step.duration_type == FIT_WKT_STEP_DURATION_TIME) {
        auto dur = node.append_child("Duration");
        dur.append_attribute("xsi:type") = "Time_t";
        dur.append_child("Seconds").text().set(static_cast<int>(step.duration_value / 1000));
    } else if (step.duration_type == FIT_WKT_STEP_DURATION_DISTANCE) {
        auto dur = node.append_child("Duration");
        dur.append_attribute("xsi:type") = "Distance_t";
        dur.append_child("Meters").text().set(static_cast<int>(step.duration_value / 100));
    } else {
        node.append_child("Duration").append_attribute("xsi:type") = "UserInitiated_t";
    }

    node.append_child("Intensity").text().set(intensityToTcx(step.intensity));

    if (step.target_type == FIT_WKT_STEP_TARGET_HEART_RATE) {
        auto tgt = node.append_child("Target");
        tgt.append_attribute("xsi:type") = "HeartRate_t";
        auto zone = tgt.append_child("HeartRateZone");
        zone.append_attribute("xsi:type") = "CustomHeartRateZone_t";
        // Both <Low> and <High> are required by the schema.
        auto lo = zone.append_child("Low");
        lo.append_attribute("xsi:type") = "HeartRateInBeatsPerMinute_t";
        lo.append_child("Value").text().set(step.has_target_low ? (int)step.target_low : 1);
        auto hi = zone.append_child("High");
        hi.append_attribute("xsi:type") = "HeartRateInBeatsPerMinute_t";
        hi.append_child("Value").text().set(step.has_target_high ? (int)step.target_high : 220);
    } else if (step.target_type == FIT_WKT_STEP_TARGET_CADENCE) {
        auto tgt = node.append_child("Target");
        tgt.append_attribute("xsi:type") = "Cadence_t";
        tgt.append_child("Low").text().set(0.0);
        tgt.append_child("High").text().set(255.0);
    } else if (step.target_type == FIT_WKT_STEP_TARGET_SPEED) {
        auto tgt = node.append_child("Target");
        tgt.append_attribute("xsi:type") = "Speed_t";
        auto zone = tgt.append_child("SpeedZone");
        zone.append_attribute("xsi:type") = "CustomSpeedZone_t";
        // <ViewAs> is required by CustomSpeedZone_t before Low/High.
        zone.append_child("ViewAs").text().set("Pace");
        zone.append_child("LowInMetersPerSecond").text().set(
            step.has_target_low ? step.target_low / 1000.0 : 0.1);
        zone.append_child("HighInMetersPerSecond").text().set(
            step.has_target_high ? step.target_high / 1000.0 : 20.0);
    } else {
        // Covers OPEN/POWER and anything else — schema uses None_t (not Open_t).
        node.append_child("Target").append_attribute("xsi:type") = "None_t";
    }
}

void writeWorkoutTcx(const WorkoutData& workout, std::ostream& out) {
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

    auto wktNode = root.append_child("Workouts").append_child("Workout");
    wktNode.append_attribute("Sport") =
        (workout.has_sport) ? sportToTcx(workout.sport) : "Other";

    if (workout.has_name)
        wktNode.append_child("Name").text().set(workout.name.c_str());

    for (const auto& step : workout.steps) {
        if (step.is_repeat) {
            auto stepNode = wktNode.append_child("Step");
            stepNode.append_attribute("xsi:type") = "Repeat_t";
            stepNode.append_child("StepId").text().set(
                static_cast<int>(step.step_index) + 1);
            stepNode.append_child("Repetitions").text().set(
                static_cast<int>(step.repetitions));
            for (const auto& child : step.children) {
                auto childNode = stepNode.append_child("Child");
                childNode.append_attribute("xsi:type") = "Step_t";
                writeStepContent(childNode, child);
            }
        } else {
            auto stepNode = wktNode.append_child("Step");
            stepNode.append_attribute("xsi:type") = "Step_t";
            writeStepContent(stepNode, step);
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
    broadcaster.AddListener(static_cast<fit::WorkoutMesgListener&>(listener));
    broadcaster.AddListener(static_cast<fit::WorkoutStepMesgListener&>(listener));

    try {
        decode.Read(&file, &broadcaster, &broadcaster, &listener);
    } catch (const fit::RuntimeException& e) {
        throw std::runtime_error(std::string("FIT decode error: ") + e.what());
    }

    return listener.data;
}
