#pragma once
#include "plan.h"
#include <string>

namespace hanson {

// Export all SOS workouts in the plan as TCX files into outdir.
// One file per week × SOS day: week_01_tue.tcx, week_01_thu.tcx, week_01_sun.tcx
// MAF warmup (15 min) and cooldown (10 min) are prepended/appended.
// MAF HR ceiling = 180 - age.
void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age);
void export_plan_to_json(const TrainingPlan& plan, const std::string& outdir, int age);

} // namespace hanson
