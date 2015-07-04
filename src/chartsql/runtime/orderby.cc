/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/orderby.h>
#include <chartsql/expressions/boolean.h>
#include <algorithm>

namespace csql {

OrderBy::OrderBy(
    std::vector<SortSpec> sort_specs,
    ScopedPtr<TableExpression> child) :
    sort_specs_(sort_specs),
    child_(std::move(child)) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }

  size_t max_sort_idx = 0;
  for (const auto& sspec : sort_specs_) {
    if (sspec.column > max_sort_idx) {
      max_sort_idx = sspec.column;
    }
  }

  const auto& child_columns = child_->numColumns();
  if (child_columns <= max_sort_idx) {
    RAISE(
        kRuntimeError,
        "can't execute ORDER BY: not enough columns in virtual table");
  }
}

// FIXPAUL this should mergesort while inserting...
// void OrderBy::execute() {
//   child_->execute();
// 
//   std::sort(rows_.begin(), rows_.end(), [this] (
//       const std::vector<SValue>& left,
//       const std::vector<SValue>& right) -> bool {
// 
//     for (const auto& sort : sort_specs_) {
//       if (sort.column >= left.size() || sort.column >= right.size()) {
// 
//         RAISE(kIndexError, "column index out of bounds");
//       }
// 
//       SValue args[2];
//       SValue res(false);
//       args[0] = left[sort.column];
//       args[1] = right[sort.column];
// 
//       expressions::eqExpr(2, args, &res);
//       if (res.getBool()) {
//         continue;
//       }
// 
//       if (sort.descending) {
//         expressions::gtExpr(2, args, &res);
//       } else {
//         expressions::ltExpr(2, args, &res);
//       }
// 
//       return res.getBool();
//     }
// 
//     /* all dimensions equal */
//     return false;
//   });
// 
//   for (auto& row : rows_) {
//     if (row.size() < columns_.size()) {
//       RAISE(kRuntimeError, "row too small");
//     }
// 
//     emitRow(row.data(), columns_.size());
//   }
// }
// 
// bool OrderBy::nextRow(SValue* row, int row_len) {
//   std::vector<SValue> row_vec;
//   for (int i = 0; i < row_len; i++) {
//     row_vec.emplace_back(row[i]);
//   }
// 
//   rows_.emplace_back(row_vec);
//   return true;
// }
// 
// size_t OrderBy::getNumCols() const {
//   return columns_.size();
// }
// 
// const std::vector<std::string>& OrderBy::getColumns() const {
//   return columns_;
// }

} // namespace csql
