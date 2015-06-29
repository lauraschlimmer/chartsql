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
#include <fnord/option.h>
#include <chartsql/qtree/ValueExpressionNode.h>

using namespace fnord;

namespace csql {

class ColumnReferenceNode : public ValueExpressionNode {
public:

  ColumnReferenceNode(const String& field_name);
  ColumnReferenceNode(size_t column_index_);

  const String& fieldName() const;

  size_t columnIndex() const;

  void setColumnIndex(size_t index);

  Vector<RefPtr<ValueExpressionNode>> arguments() const override;

  RefPtr<QueryTreeNode> deepCopy() const override;

protected:
  String field_name_;
  Option<size_t> column_index_;
};

} // namespace csql
