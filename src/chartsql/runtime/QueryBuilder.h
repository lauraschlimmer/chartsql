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
#include <stx/stdtypes.h>
#include <chartsql/qtree/ChartStatementNode.h>
#include <chartsql/runtime/ValueExpressionBuilder.h>
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/charts/ChartStatement.h>
#include <chartsql/runtime/ValueExpression.h>

using namespace stx;

namespace csql {

class QueryBuilder : public RefCounted {
public:

  QueryBuilder(
      RefPtr<ValueExpressionBuilder> scalar_exp_builder,
      RefPtr<TableExpressionBuilder> table_exp_builder);

  ScopedPtr<ValueExpression> buildValueExpression(
      RefPtr<ValueExpressionNode> expression);

  ScopedPtr<TableExpression> buildTableExpression(
      RefPtr<TableExpressionNode> expression,
      RefPtr<TableProvider> tables,
      Runtime* runtime);

  ScopedPtr<ChartStatement> buildChartStatement(
      RefPtr<ChartStatementNode> node,
      RefPtr<TableProvider> tables,
      Runtime* runtime);

protected:
  RefPtr<ValueExpressionBuilder> scalar_exp_builder_;
  RefPtr<TableExpressionBuilder> table_exp_builder_;
};

} // namespace csql
