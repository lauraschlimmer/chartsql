/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/defaultruntime.h>

namespace csql {

class GroupBy : public TableExpression {
public:

  GroupBy(
      ScopedPtr<TableExpression> source,
      Vector<ScopedPtr<ValueExpression>> select_expressions,
      Vector<ScopedPtr<ValueExpression>> group_expressions);

  ~GroupBy();

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  virtual Vector<String> columnNames() const override;

  virtual size_t numColunns() const override;

protected:

  bool nextRow(int argc, const SValue* argv);

  ScopedPtr<TableExpression> source_;
  Vector<ScopedPtr<ValueExpression>> select_exprs_;
  Vector<ScopedPtr<ValueExpression>> group_exprs_;
  HashMap<String, Vector<ValueExpression::Instance>> groups_;
  ScratchMemory scratch_;
};

}
