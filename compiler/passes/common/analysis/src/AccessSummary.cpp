#include "common/analysis/AccessAnalysis.h"

namespace llvm::memoir {

/*
 * Access Summary base class implementation
 */
AccessSummary::AccessSummary(CallInst &call_inst, PointsToInfo points_to_info)
  : call_inst(call_inst),
    points_to_info(points_to_info) {
  // Do nothing.
}

bool AccessSummary::isMust() {
  return (this->points_to_info == PointsToInfo::Must);
}

bool AccessSummary::isMay() {
  return (this->points_to_info == PointsToInfo::May);
}

PointsToInfo AccessSummary::getPointsToInfo() {
  return this->points_to_info;
}

bool AccessSummary::isMust() {
  return (this->access_info == AccessInfo::Read);
}

bool AccessSummary::isMay() {
  return (this->access_info == AccessInfo::Write);
}

AccessInfo AccessSummary::getAccessInfo() {
  return this->access_info;
}

/*
 * Read Summary implementation
 */
ReadSummary::ReadSummary(llvm::CallInst &call_inst,
                         PointsToInfo points_to_info,
                         FieldSummary &field)
  : field(field),
    AccessSummary(call_inst, points_to_info, AccessInfo::Read) {
  // Do nothing.
}

FieldSummary &ReadSummary::getField() {
  return this->field;
}

TypeSummary &ReadSummary::getType() {
  return this->field.getType();
}

/*
 * Write Summary implementation
 */
WriteSummary::WriteSummary(llvm::CallInst &call_inst,
                           PointsToInfo points_to_info,
                           FieldSummary &field)
  : field(field),
    AccessSummary(call_inst, points_to_info, AccessInfo::Write) {
  // Do nothing.
}

llvm::Value &WriteSummary::getValueWritten() {
  return this->value_written;
}

FieldSummary &WriteSummary::getField() {
  return this->field;
}

TypeSummary &WriteSummary::getType() {
  return this->field.getType();
}

/*
 * May Read Summary implementation
 */
MayReadSummary::MayReadSummary(
    llvm::CallInst &call_inst,
    std::unordered_set<ReadSummary *> &may_read_summaries)
  : may_read_summaries(may_read_summaries),
    AccessSummary(call_inst, PointsToInfo::May, AccessInfo::Read) {
  // Do nothing.
}

MayReadSummary::iterator MayReadSummary::begin() {
  const auto &const_may_read_summaries = may_read_summaries;
  auto iter = const_may_read_summaries.begin();
}

MayReadSummary::iterator MayReadSummary::end() {
  const auto &const_may_read_summaries = may_read_summaries;
  auto iter = const_may_read_summaries.end();
}

/*
 * May Write Summary implementation
 */
MayWriteSummary::MayWriteSummary(
    llvm::CallInst &call_inst,
    std::unordered_set<WriteSummary *> &may_write_summaries)
  : may_write_summaries(may_write_summaries),
    AccessSummary(call_inst, PointsToInfo::May, AccessInfo::Write) {
  // Do nothing.
}

MayWriteSummary::iterator MayWriteSummary::begin() {
  const auto &const_may_write_summaries = may_write_summaries;
  auto iter = const_may_write_summaries.begin();
}

MayWriteSummary::iterator MayWriteSummary::end() {
  const auto &const_may_write_summaries = may_write_summaries;
  auto iter = const_may_write_summaries.end();
}

} // namespace llvm::memoir
