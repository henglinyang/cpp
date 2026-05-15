#pragma once
#include "plan.h"
#include <string>

namespace first {

// Export all workouts in a training plan as TCX files into outdir.
// One file per week × key run: week_16_kr1.tcx, week_16_kr2.tcx, etc.
// MAF warmup (15 min) and cooldown (10 min) are prepended/appended to every
// workout; MAF HR ceiling = 180 - age.
void export_plan_to_tcx(const TrainingPlan& plan, const std::string& outdir, int age);
void export_plan_to_json(const TrainingPlan& plan, const std::string& outdir, int age);

} // namespace first
