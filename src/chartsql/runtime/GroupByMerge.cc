/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/GroupByMerge.h>
#include <chartsql/runtime/EmptyTable.h>

namespace csql {

GroupByMerge::GroupByMerge(
    Vector<ScopedPtr<GroupByExpression>> sources) :
    sources_(std::move(sources)) {
  if (sources_.size() == 0) {
    RAISE(kRuntimeError, "GROUP MERGE must have at least one source table");
  }

  size_t ncols = size_t(-1);

  for (const auto& cur : sources_) {
    if (dynamic_cast<EmptyTable*>(cur.get())) {
      continue;
    }

    auto tcols = cur->numColumns();
    if (ncols != size_t(-1) && ncols != tcols) {
      RAISE(
          kRuntimeError,
          "GROUP MERGE tables return different number of columns");
    }

    ncols = tcols;
  }
}

void GroupByMerge::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  HashMap<String, Vector<ValueExpression::Instance>> groups;
  ScratchMemory scratch;

  try {
    for (auto& source : sources_) {
      HashMap<String, Vector<ValueExpression::Instance>> tgroups;
      ScratchMemory tscratch;
      source->accumulate(&tgroups, &tscratch, context);
      source->mergeResult(&tgroups, &groups, &scratch);
    }

    sources_[0]->getResult(&groups, fn);
  } catch (...) {
    sources_[0]->freeResult(&groups);
    throw;
  }

  sources_[0]->freeResult(&groups);
}

Vector<String> GroupByMerge::columnNames() const {
  return sources_[0]->columnNames();
}

size_t GroupByMerge::numColumns() const {
  return sources_[0]->numColumns();
}

}
