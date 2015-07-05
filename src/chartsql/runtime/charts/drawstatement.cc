/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/runtime.h>
#include <chartsql/runtime/charts/domainconfig.h>
#include <chartsql/runtime/charts/drawstatement.h>
#include <chartsql/runtime/charts/areachartbuilder.h>
#include <chartsql/runtime/charts/barchartbuilder.h>
#include <chartsql/runtime/charts/linechartbuilder.h>
#include <chartsql/runtime/charts/pointchartbuilder.h>

namespace csql {

DrawStatement::DrawStatement(
    RefPtr<DrawNode> node,
    Vector<ScopedPtr<TableExpression>> sources,
    Runtime* runtime) :
    node_(node),
    sources_(std::move(sources)),
    runtime_(runtime) {}

void DrawStatement::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  fnord::chart::Drawable* chart = nullptr;

  switch (node_->chartType()) {
    case DrawNode::ChartType::AREACHART:
      chart = executeWithChart<AreaChartBuilder>(context, fn);
      break;
    case DrawNode::ChartType::BARCHART:
      chart = executeWithChart<BarChartBuilder>(context, fn);
      break;
    case DrawNode::ChartType::LINECHART:
      chart = executeWithChart<LineChartBuilder>(context, fn);
      break;
    case DrawNode::ChartType::POINTCHART:
      chart = executeWithChart<PointChartBuilder>(context, fn);
      break;
  }

  applyDomainDefinitions(chart);
  applyTitle(chart);
  applyAxisDefinitions(chart);
  applyGrid(chart);
  applyLegend(chart);
}

void DrawStatement::applyAxisDefinitions(fnord::chart::Drawable* chart) const {
  for (const auto& child : node_->ast()->getChildren()) {
    if (child->getType() != ASTNode::T_AXIS ||
        child->getChildren().size() < 1 ||
        child->getChildren()[0]->getType() != ASTNode::T_AXIS_POSITION) {
      continue;
    }

    fnord::chart::AxisDefinition* axis = nullptr;

    if (child->getChildren().size() < 1) {
      RAISE(kRuntimeError, "corrupt AST: AXIS has < 1 child");
    }

    switch (child->getChildren()[0]->getToken()->getType()) {
      case Token::T_TOP:
        axis = chart->addAxis(fnord::chart::AxisDefinition::TOP);
        break;

      case Token::T_RIGHT:
        axis = chart->addAxis(fnord::chart::AxisDefinition::RIGHT);
        break;

      case Token::T_BOTTOM:
        axis = chart->addAxis(fnord::chart::AxisDefinition::BOTTOM);
        break;

      case Token::T_LEFT:
        axis = chart->addAxis(fnord::chart::AxisDefinition::LEFT);
        break;

      default:
        RAISE(kRuntimeError, "corrupt AST: invalid axis position");
    }

    for (int i = 1; i < child->getChildren().size(); ++i) {
      auto prop = child->getChildren()[i];

      if (prop->getType() == ASTNode::T_PROPERTY &&
          prop->getToken() != nullptr &&
          *prop->getToken() == Token::T_TITLE &&
          prop->getChildren().size() == 1) {
        auto axis_title = runtime_->evaluateStaticExpression(
            prop->getChildren()[0]);
        axis->setTitle(axis_title.toString());
        continue;
      }

      if (prop->getType() == ASTNode::T_AXIS_LABELS) {
        applyAxisLabels(prop, axis);
      }
    }
  }
}

void DrawStatement::applyAxisLabels(
    ASTNode* ast,
    fnord::chart::AxisDefinition* axis) const {
  for (const auto& prop : ast->getChildren()) {
    if (prop->getType() != ASTNode::T_PROPERTY ||
        prop->getToken() == nullptr) {
      continue;
    }

    switch (prop->getToken()->getType()) {
      case Token::T_INSIDE:
        axis->setLabelPosition(fnord::chart::AxisDefinition::LABELS_INSIDE);
        break;
      case Token::T_OUTSIDE:
        axis->setLabelPosition(fnord::chart::AxisDefinition::LABELS_OUTSIDE);
        break;
      case Token::T_OFF:
        axis->setLabelPosition(fnord::chart::AxisDefinition::LABELS_OFF);
        break;
      case Token::T_ROTATE: {
        if (prop->getChildren().size() != 1) {
          RAISE(kRuntimeError, "corrupt AST: ROTATE has no children");
        }

        auto rot = runtime_->evaluateStaticExpression(
            prop->getChildren()[0]);
        axis->setLabelRotation(rot.getValue<double>());
        break;
      }
      default:
        RAISE(kRuntimeError, "corrupt AST: LABELS has invalid token");
    }
  }
}

void DrawStatement::applyDomainDefinitions(
    fnord::chart::Drawable* chart) const {
  for (const auto& child : node_->ast()->getChildren()) {
    bool invert = false;
    bool logarithmic = false;
    ASTNode* min_expr = nullptr;
    ASTNode* max_expr = nullptr;

    if (child->getType() != ASTNode::T_DOMAIN) {
      continue;
    }

    if (child->getToken() == nullptr) {
      RAISE(kRuntimeError, "corrupt AST: DOMAIN has no token");
    }

    fnord::chart::AnyDomain::kDimension dim;
    switch (child->getToken()->getType()) {
      case Token::T_XDOMAIN:
        dim = fnord::chart::AnyDomain::DIM_X;
        break;
      case Token::T_YDOMAIN:
        dim = fnord::chart::AnyDomain::DIM_Y;
        break;
      case Token::T_ZDOMAIN:
        dim = fnord::chart::AnyDomain::DIM_Z;
        break;
      default:
        RAISE(kRuntimeError, "corrupt AST: DOMAIN has invalid token");
    }

    for (const auto& domain_prop : child->getChildren()) {
      switch (domain_prop->getType()) {
        case ASTNode::T_DOMAIN_SCALE: {
          auto min_max_expr = domain_prop->getChildren();
          if (min_max_expr.size() != 2 ) {
            RAISE(kRuntimeError, "corrupt AST: invalid DOMAIN SCALE");
          }
          min_expr = min_max_expr[0];
          max_expr = min_max_expr[1];
          break;
        }

        case ASTNode::T_PROPERTY: {
          if (domain_prop->getToken() != nullptr) {
            switch (domain_prop->getToken()->getType()) {
              case Token::T_INVERT:
                invert = true;
                continue;
              case Token::T_LOGARITHMIC:
                logarithmic = true;
                continue;
              default:
                break;
            }
          }

          RAISE(kRuntimeError, "corrupt AST: invalid DOMAIN property");
          break;
        }

        default:
          RAISE(kRuntimeError, "corrupt AST: unexpected DOMAIN child");

      }
    }

    DomainConfig domain_config(chart, dim);
    domain_config.setInvert(invert);
    domain_config.setLogarithmic(logarithmic);
    if (min_expr != nullptr && max_expr != nullptr) {
      domain_config.setMin(runtime_->evaluateStaticExpression(min_expr));
      domain_config.setMax(runtime_->evaluateStaticExpression(max_expr));
    }
  }
}

void DrawStatement::applyTitle(fnord::chart::Drawable* chart) const {
  for (const auto& child : node_->ast()->getChildren()) {
    if (child->getType() != ASTNode::T_PROPERTY ||
        child->getToken() == nullptr || !(
        child->getToken()->getType() == Token::T_TITLE ||
        child->getToken()->getType() == Token::T_SUBTITLE)) {
      continue;
    }

    if (child->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST: [SUB]TITLE has != 1 child");
    }

    auto title_eval = runtime_->evaluateStaticExpression(
        child->getChildren()[0]);
    auto title_str = title_eval.toString();

    switch (child->getToken()->getType()) {
      case Token::T_TITLE:
        chart->setTitle(title_str);
        break;
      case Token::T_SUBTITLE:
        chart->setSubtitle(title_str);
        break;
      default:
        break;
    }
  }
}

void DrawStatement::applyGrid(fnord::chart::Drawable* chart) const {
  ASTNode* grid = nullptr;

  for (const auto& child : node_->ast()->getChildren()) {
    if (child->getType() == ASTNode::T_GRID) {
      grid = child;
      break;
    }
  }

  if (!grid) {
    return;
  }

  bool horizontal = false;
  bool vertical = false;

  for (const auto& prop : grid->getChildren()) {
    if (prop->getType() == ASTNode::T_PROPERTY && prop->getToken() != nullptr) {
      switch (prop->getToken()->getType()) {
        case Token::T_HORIZONTAL:
          horizontal = true;
          break;
        case Token::T_VERTICAL:
          vertical = true;
          break;
        default:
          RAISE(kRuntimeError, "corrupt AST: invalid GRID property");
      }
    }
  }

  if (horizontal) {
    chart->addGrid(fnord::chart::GridDefinition::GRID_HORIZONTAL);
  }

  if (vertical) {
    chart->addGrid(fnord::chart::GridDefinition::GRID_VERTICAL);
  }
}

void DrawStatement::applyLegend(fnord::chart::Drawable* chart) const {
  ASTNode* legend = nullptr;

  for (const auto& child : node_->ast()->getChildren()) {
    if (child->getType() == ASTNode::T_LEGEND) {
      legend = child;
      break;
    }
  }

  if (!legend) {
    return;
  }


  fnord::chart::LegendDefinition::kVerticalPosition vert_pos =
      fnord::chart::LegendDefinition::LEGEND_BOTTOM;
  fnord::chart::LegendDefinition::kHorizontalPosition horiz_pos =
      fnord::chart::LegendDefinition::LEGEND_LEFT;
  fnord::chart::LegendDefinition::kPlacement placement =
      fnord::chart::LegendDefinition::LEGEND_OUTSIDE;
  std::string title;

  for (const auto& prop : legend->getChildren()) {
    if (prop->getType() == ASTNode::T_PROPERTY && prop->getToken() != nullptr) {
      switch (prop->getToken()->getType()) {
        case Token::T_TOP:
          vert_pos = fnord::chart::LegendDefinition::LEGEND_TOP;
          break;
        case Token::T_RIGHT:
          horiz_pos = fnord::chart::LegendDefinition::LEGEND_RIGHT;
          break;
        case Token::T_BOTTOM:
          vert_pos = fnord::chart::LegendDefinition::LEGEND_BOTTOM;
          break;
        case Token::T_LEFT:
          horiz_pos = fnord::chart::LegendDefinition::LEGEND_LEFT;
          break;
        case Token::T_INSIDE:
          placement = fnord::chart::LegendDefinition::LEGEND_INSIDE;
          break;
        case Token::T_OUTSIDE:
          placement = fnord::chart::LegendDefinition::LEGEND_OUTSIDE;
          break;
        case Token::T_TITLE: {
          if (prop->getChildren().size() != 1) {
            RAISE(kRuntimeError, "corrupt AST: TITLE has no children");
          }

          auto sval = runtime_->evaluateStaticExpression(
              prop->getChildren()[0]);

          title = sval.toString();
          break;
        }
        default:
          RAISE(
              kRuntimeError,
              "corrupt AST: LEGEND has invalid property");
      }
    }
  }

  chart->addLegend(vert_pos, horiz_pos, placement, title);
}

}
