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
#include <chartsql/qtree/ScalarExpressionNode.h>

using namespace fnord;

namespace csql {

class CallExpressionNode : public ScalarExpressionNode {
public:

  CallExpressionNode(
      const String& symbol,
      Vector<RefPtr<ScalarExpressionNode>> arguments);

  Vector<RefPtr<ScalarExpressionNode>> arguments() const override;

  const String& symbol() const;

protected:
  String symbol_;
  Vector<RefPtr<ScalarExpressionNode>> arguments_;
};

} // namespace csql
