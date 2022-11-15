#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

/*
 * Access Summary base class implementation
 */
AccessSummary::AccessSummary(CallInst &call_inst,
                             set<FieldSummary *> &fields_accessed,
                             PointsToInfo points_to_info,
                             AccessInfo access_info)
  : call_inst(call_inst),
    fields_accessed(fields_accessed),
    points_to_info(points_to_info),
    access_info(access_info) {
  // Do nothing.
}

bool AccessSummary::isMust() const {
  return (this->fields_accessed.size() == 1);
}

bool AccessSummary::isMay() const {
  return !(this->isMust());
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

TypeSummary &AccessSummary::getType() const {
  auto first_field = *(this->fields);
  return first_field->getType();
}

FieldSummary *AccessSummary::getSingleField() const {
  if (this->isMay()) {
    return nullptr;
  }

  return *(this->begin());
}

AccessSummary::iterator AccessSummary::begin() {
  return this->fields_accessed.begin();
}

AccessSummary::iterator AccessSummary::end() {
  return this->fields_accessed.end();
}

AccessSummary::const_iterator AccessSummary::cbegin() const {
  return this->fields_accessed.cbegin();
}

AccessSummary::const_iterator AccessSummary::cend() const {
  return this->fields_accessed.cend();
}

/*
 * Read Summary implementation
 */
ReadSummary::ReadSummary(llvm::CallInst &call_inst,
                         set<FieldSummary *> &may_read_fields)
  : AccessSummary(call_inst, may_read_fields, AccessInfo::READ) {
  // Do nothing.
}

ReadSummary::ReadSummary(llvm::CallInst &call_inst,
                         FieldSummary &must_read_field)
  : AccessSummary(call_inst, { &must_read_field }, AccessInfo::READ) {
  // Do nothing.
}

/*
 *  Write Summary implementation
 */
WriteSummary::WriteSummary(llvm::CallInst &call_inst,
                           set<FieldSummary *> &may_write_fields,
                           llvm::Value &value_written)
  : value_written(value_written),
    AccessSummary(call_inst, may_write_fields, AccessInfo::WRITE) {
  // Do nothing.
}

WriteSummary::WriteSummary(llvm::CallInst &call_inst,
                           FieldSummary &must_write_field,
                           llvm::Value &value_written)
  : value_written(value_written),
    AccessSummary(call_inst, { &must_write_field }, AccessInfo::WRITE) {
  // Do nothing.
}

TypeSummary &WriteSummary::getValueWritten() const {
  return this->value_written;
}

} // namespace llvm::memoir
