/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/protobuf/MessageSchema.h>
#include <chartsql/qtree/SequentialScanNode.h>
#include <chartsql/runtime/compile.h>
#include <chartsql/runtime/defaultruntime.h>
#include <chartsql/runtime/TableExpression.h>
#include <cstable/CSTableReader.h>

using namespace fnord;

namespace csql {

class CSTableScan : public TableExpression {
public:

  CSTableScan(
      RefPtr<SequentialScanNode> stmt,
      cstable::CSTableReader&& cstable,
      Runtime* runtime);

  virtual Vector<String> columnNames() const override;

  virtual size_t numColunns() const override;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

protected:

  struct ColumnRef {
    ColumnRef(RefPtr<cstable::ColumnReader> r, size_t i);
    RefPtr<cstable::ColumnReader> reader;
    size_t index;
  };

  struct ExpressionRef {
    ExpressionRef(
        size_t _rep_level,
        ScopedPtr<ValueExpression> _compiled,
        ScratchMemory* scratch);

    ExpressionRef(ExpressionRef&& other);
    ~ExpressionRef();

    size_t rep_level;
    ScopedPtr<ValueExpression> compiled;
    ValueExpression::Instance instance;
  };

  void scan(
      Function<bool (int argc, const SValue* argv)> fn);

  void findColumns(
      RefPtr<ValueExpressionNode> expr,
      Set<String>* column_names) const;

  void resolveColumns(RefPtr<ValueExpressionNode> expr) const;

  uint64_t findMaxRepetitionLevel(RefPtr<ValueExpressionNode> expr) const;

  void fetch();

  ScratchMemory scratch_;
  cstable::CSTableReader cstable_;
  HashMap<String, ColumnRef> columns_;
  Vector<ExpressionRef> select_list_;
  size_t colindex_;
  AggregationStrategy aggr_strategy_;
};


} // namespace csql
