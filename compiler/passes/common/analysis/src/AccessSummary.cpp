#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * Access Summary base class implementation
 */
AccessSummary::AccessSummary(CallInst &call_inst,
                             PointsToInfo points_to_info,
                             AccessInfo access_info)
  : call_inst(call_inst),
    points_to_info(points_to_info),
    access_info(access_info) {
  // Do nothing.
}

bool AccessSummary::isMust() const {
  return (this->points_to_info == PointsToInfo::MUST);
}

bool AccessSummary::isMay() const {
  return (this->points_to_info == PointsToInfo::MAY);
}

PointsToInfo AccessSummary::getPointsToInfo() const {
  return this->points_to_info;
}

bool AccessSummary::isRead() const {
  return (this->access_info == AccessInfo::READ);
}

bool AccessSummary::isWrite() const {
  return (this->access_info == AccessInfo::WRITE);
}

AccessInfo AccessSummary::getAccessInfo() const {
  return this->access_info;
}

llvm::CallInst &AccessSummary::getCallInst() const {
  return this->call_inst;
}

/*
 * Read Summary implementation
 */
MustReadSummary::MustReadSummary(llvm::CallInst &call_inst, FieldSummary &field)
  : field(field),
    AccessSummary(call_inst, PointsToInfo::MUST, AccessInfo::READ) {
  // Do nothing.
}

FieldSummary &MustReadSummary::getField() const {
  return this->field;
}

TypeSummary &MustReadSummary::getType() const {
  return this->field.getType();
}

/*
 * Write Summary implementation
 */
MustWriteSummary::MustWriteSummary(llvm::CallInst &call_inst,
                                   FieldSummary &field,
                                   llvm::Value &value_written)
  : field(field),
    value_written(value_written),
    AccessSummary(call_inst, PointsToInfo::MUST, AccessInfo::WRITE) {
  // Do nothing.
}

llvm::Value &MustWriteSummary::getValueWritten() const {
  return this->value_written;
}

FieldSummary &MustWriteSummary::getField() const {
  return this->field;
}

TypeSummary &MustWriteSummary::getType() const {
  return this->field.getType();
}

/*
 * May Read Summary implementation
 */
MayReadSummary::MayReadSummary(
    llvm::CallInst &call_inst,
    TypeSummary &type,
    std::unordered_set<FieldSummary *> &may_read_summaries)
  : type(type),
    may_read_summaries(may_read_summaries),
    AccessSummary(call_inst, PointsToInfo::MAY, AccessInfo::READ) {
  // Do nothing.
}

MayReadSummary::iterator MayReadSummary::begin() {
  return this->may_read_summaries.begin();
}

MayReadSummary::iterator MayReadSummary::end() {
  return this->may_read_summaries.end();
}

MayReadSummary::const_iterator MayReadSummary::cbegin() const {
  return this->may_read_summaries.cbegin();
}

MayReadSummary::const_iterator MayReadSummary::cend() const {
  return this->may_read_summaries.cend();
}

TypeSummary &MayReadSummary::getType() const {
  return this->type;
}

/*
 * May Write Summary implementation
 */
MayWriteSummary::MayWriteSummary(
    llvm::CallInst &call_inst,
    TypeSummary &type,
    std::unordered_set<FieldSummary *> &may_write_summaries,
    llvm::Value &value_written)
  : type(type),
    may_write_summaries(may_write_summaries),
    value_written(value_written),
    AccessSummary(call_inst, PointsToInfo::MAY, AccessInfo::WRITE) {
  // Do nothing.
}

MayWriteSummary::iterator MayWriteSummary::begin() {
  return may_write_summaries.begin();
}

MayWriteSummary::iterator MayWriteSummary::end() {
  return may_write_summaries.end();
}

MayWriteSummary::const_iterator MayWriteSummary::cbegin() const {
  return may_write_summaries.cbegin();
}

MayWriteSummary::const_iterator MayWriteSummary::cend() const {
  return may_write_summaries.cend();
}

TypeSummary &MayWriteSummary::getType() const {
  return this->type;
}

} // namespace llvm::memoir
