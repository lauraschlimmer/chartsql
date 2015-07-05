/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQLEXTENSIONS_LINECHARTBUILDER_H
#define _FNORDMETRIC_SQLEXTENSIONS_LINECHARTBUILDER_H
#include <chartsql/runtime/charts/chartbuilder.h>
#include <fnord/charts/linechart.h>

namespace csql {
class DrawStatement;

class LineChartBuilder : public ChartBuilder {
public:
  LineChartBuilder(fnord::chart::Canvas* canvas, RefPtr<DrawStatementNode> draw_stmt);
  fnord::chart::Drawable* getChart() const override;
  std::string chartName() const override;
protected:
  fnord::chart::Drawable* findChartType() const;
  void setLabels(fnord::chart::LineChart* chart) const;
};

}
#endif
