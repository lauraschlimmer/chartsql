/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/wallclock.h>
#include <stx/test/unittest.h>
#include "csql/runtime/defaultruntime.h"
#include "csql/qtree/SequentialScanNode.h"
#include "csql/qtree/ColumnReferenceNode.h"
#include "csql/qtree/CallExpressionNode.h"
#include "csql/qtree/LiteralExpressionNode.h"
#include "csql/CSTableScanProvider.h"

using namespace stx;
using namespace csql;

UNIT_TEST(RuntimeTest);

TEST_CASE(RuntimeTest, TestStaticExpression, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto expr = mkRef(
      new csql::CallExpressionNode(
          "add",
          {
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(1))),
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(2))),
          }));

  auto t0 = WallClock::unixMicros();
  SValue out;
  for (int i = 0; i < 1000; ++i) {
    out = runtime->evaluateStaticExpression(expr.get());
  }
  auto t1 = WallClock::unixMicros();

  EXPECT_EQ(out.getInteger(), 3);
});

TEST_CASE(RuntimeTest, TestExecuteIfStatement, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto out_a = runtime->evaluateStaticExpression("if(1 = 1, 42, 23)");
  EXPECT_EQ(out_a.getInteger(), 42);

  auto out_b = runtime->evaluateStaticExpression("if(1 = 2, 42, 23)");
  EXPECT_EQ(out_b.getInteger(), 23);

  {
    auto v = runtime->evaluateStaticExpression("if(1 = 1, 'fnord', 'blah')");
    EXPECT_EQ(v.toString(), "fnord");
  }

  {
    auto v = runtime->evaluateStaticExpression("if(1 = 2, 'fnord', 'blah')");
    EXPECT_EQ(v.toString(), "blah");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = 'blah', 1, 2)");
    EXPECT_EQ(v.toString(), "2");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = 'fnord', 1, 2)");
    EXPECT_EQ(v.toString(), "1");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = '', 1, 2)");
    EXPECT_EQ(v.toString(), "2");
  }
});

TEST_CASE(RuntimeTest, TestSimpleCSTableAggregate, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "testtable",
          "src/csql/testdata/testtbl.cst"));

  ResultList result;
  auto query = R"(select count(1) from testtable;)";
  auto qplan = runtime->buildQueryPlan(query, estrat.get());
  runtime->executeStatement(qplan->buildStatement(0), &result);
  EXPECT_EQ(result.getNumColumns(), 1);
  EXPECT_EQ(result.getNumRows(), 1);
  EXPECT_EQ(result.getRow(0)[0], "213");
});

TEST_CASE(RuntimeTest, TestNestedCSTableAggregate, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "testtable",
          "src/csql/testdata/testtbl.cst"));

  ResultList result;
  auto query = R"(select count(event.search_query.time) from testtable;)";
  auto qplan = runtime->buildQueryPlan(query, estrat.get());
  runtime->executeStatement(qplan->buildStatement(0), &result);
  EXPECT_EQ(result.getNumColumns(), 1);
  EXPECT_EQ(result.getNumRows(), 1);
  EXPECT_EQ(result.getRow(0)[0], "704");
});

TEST_CASE(RuntimeTest, TestWithinRecordCSTableAggregate, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "testtable",
          "src/csql/testdata/testtbl.cst"));

  {
    ResultList result;
    auto query = R"(select sum(event.search_query.num_result_items) from testtable;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 1);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "24793");
  }

  {
    ResultList result;
    auto query = R"(select sum(count(event.search_query.result_items.position) WITHIN RECORD) from testtable;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 1);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "24793");
  }

  ResultList result;
  auto query = R"(
      select
        sum(event.search_query.num_result_items) WITHIN RECORD,
        count(event.search_query.result_items.position) WITHIN RECORD
      from testtable;)";
  auto qplan = runtime->buildQueryPlan(query, estrat.get());
  runtime->executeStatement(qplan->buildStatement(0), &result);
  EXPECT_EQ(result.getNumColumns(), 2);
  EXPECT_EQ(result.getNumRows(), 213);

  size_t s = 0;
  for (size_t i = 0; i < result.getNumRows(); ++i) {
    auto r1 = result.getRow(i)[0];
    if (r1 == "NULL") {
      r1 = "0";
    }

    auto r2 = result.getRow(i)[1];
    if (r2 == "NULL") {
      r2 = "0";
    }

    EXPECT_EQ(r1, r2);
    s += std::stoull(r1);
  }

  EXPECT_EQ(s, 24793);
});

TEST_CASE(RuntimeTest, TestMultiLevelNestedCSTableAggregate, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "testtable",
          "src/csql/testdata/testtbl.cst"));

  {
    ResultList result;
    auto query = R"(
        select
          1,
          event.search_query.time,
          event.search_query.num_result_items,
          event.search_query.result_items.position
        from testtable;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumRows(), 24866);
  }

  {
    ResultList result;
    auto query = R"(
        select
          count(time),
          count(event.search_query.time),
          sum(event.search_query.num_result_items),
          sum(count(event.search_query.result_items.position) WITHIN RECORD)
        from testtable;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);

    EXPECT_EQ(result.getNumColumns(), 4);
    auto cols = result.getColumns();
    EXPECT_EQ(cols[0], "count(time)");
    EXPECT_EQ(cols[1], "count(event.search_query.time)");
    EXPECT_EQ(cols[2], "sum(event.search_query.num_result_items)");
    EXPECT_EQ(
        cols[3],
        "sum(count(event.search_query.result_items.position) WITHIN RECORD)");

    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "213");
    EXPECT_EQ(result.getRow(0)[1], "704");
    EXPECT_EQ(result.getRow(0)[2], "24793");
    EXPECT_EQ(result.getRow(0)[3], "24793");
  }

  {
    ResultList result;
    auto query = R"(
        select
          count(time),
          count(event.search_query.time),
          sum(event.search_query.num_result_items),
          sum(count(event.search_query.result_items.position) WITHIN RECORD),
          (
            count(time) +
            count(event.search_query.time) +
            sum(event.search_query.num_result_items) +
            sum(count(event.search_query.result_items.position) WITHIN RECORD)
          )
        from testtable
        group by 1;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 5);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "213");
    EXPECT_EQ(result.getRow(0)[1], "704");
    EXPECT_EQ(result.getRow(0)[2], "24793");
    EXPECT_EQ(result.getRow(0)[3], "24793");
    EXPECT_EQ(result.getRow(0)[4], "50503");
  }
});


TEST_CASE(RuntimeTest, TestMultiLevelNestedCSTableAggrgateWithGroup, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "testtable",
          "src/csql/testdata/testtbl.cst"));

  {
    ResultList result;
    auto query = R"(
        select
          sum(count(event.search_query.result_items.position) WITHIN RECORD),
          sum(sum(if(event.search_query.result_items.clicked, 1, 0)) WITHIN RECORD)
        from testtable
        where event.search_query.result_items.position = 6;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 2);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "688");
    EXPECT_EQ(result.getRow(0)[1], "2");
  }

  {
    ResultList result;
    auto query = R"(
        select
          sum(count(event.search_query.result_items.position) WITHIN RECORD),
          sum(sum(if(event.search_query.result_items.clicked, 1, 0)) WITHIN RECORD)
        from testtable
        where event.search_query.result_items.position = 9;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 2);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "679");
    EXPECT_EQ(result.getRow(0)[1], "4");
  }

  {
    ResultList result;
    auto query = R"(
        select
          event.search_query.result_items.position,
          sum(count(event.search_query.result_items.position) WITHIN RECORD),
          sum(sum(if(event.search_query.result_items.clicked, 1, 0)) WITHIN RECORD)
        from testtable
        group by event.search_query.result_items.position
        order by event.search_query.result_items.position ASC
        LIMIT 10;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);

    EXPECT_EQ(result.getNumColumns(), 3);
    auto cols = result.getColumns();
    EXPECT_EQ(cols[0], "event.search_query.result_items.position");
    EXPECT_EQ(cols[1], "sum(count(event.search_query.result_items.position) WITHIN RECORD)");
    EXPECT_EQ(cols[2], "sum(sum(if(event.search_query.result_items.clicked, 1, 0)) WITHIN RECORD)");

    EXPECT_EQ(result.getNumRows(), 10);
    EXPECT_EQ(result.getRow(6)[0], "6");
    EXPECT_EQ(result.getRow(6)[1], "688");
    EXPECT_EQ(result.getRow(6)[2], "2");
  }
});

TEST_CASE(RuntimeTest, TestTableNamesWithDots, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(
      new CSTableScanProvider(
          "test.tbl",
          "src/csql/testdata/testtbl.cst"));

  {
    ResultList result;
    auto query = R"(select count(1) from 'test.tbl';)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 1);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "213");
  }

  {
    ResultList result;
    auto query = R"(select count(1) from `test.tbl`;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 1);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "213");
  }

  {
    ResultList result;
    auto query = R"(select count(1) from test.tbl;)";
    auto qplan = runtime->buildQueryPlan(query, estrat.get());
    runtime->executeStatement(qplan->buildStatement(0), &result);
    EXPECT_EQ(result.getNumColumns(), 1);
    EXPECT_EQ(result.getNumRows(), 1);
    EXPECT_EQ(result.getRow(0)[0], "213");
  }

});

TEST_CASE(RuntimeTest, SelectFloatIntegerDivision, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  {
    auto v = runtime->evaluateStaticExpression("1 / 5");
    EXPECT_EQ(v.toString(), "0.200000");
  }
});

TEST_CASE(RuntimeTest, SelectFloatIntegerMultiplication, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  {
    auto v = runtime->evaluateStaticExpression("10 * 5");
    EXPECT_EQ(v.toString(), "50");
  }

  {
    auto v = runtime->evaluateStaticExpression("10 * 5.0");
    EXPECT_EQ(v.toString(), "50.000000");
  }

  {
    auto v = runtime->evaluateStaticExpression("10.0 * 5");
    EXPECT_EQ(v.toString(), "50.000000");
  }
});

TEST_CASE(RuntimeTest, SelectFloatIntegerAddition, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  {
    auto v = runtime->evaluateStaticExpression("10 + 5");
    EXPECT_EQ(v.toString(), "15");
  }

  {
    auto v = runtime->evaluateStaticExpression("10 + 5.0");
    EXPECT_EQ(v.toString(), "15.000000");
  }

  {
    auto v = runtime->evaluateStaticExpression("10.0 + 5");
    EXPECT_EQ(v.toString(), "15.000000");
  }
});

TEST_CASE(RuntimeTest, SelectFloatIntegerSubtraction, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  {
    auto v = runtime->evaluateStaticExpression("10 - 5");
    EXPECT_EQ(v.toString(), "5");
  }

  {
    auto v = runtime->evaluateStaticExpression("10 - 5.0");
    EXPECT_EQ(v.toString(), "5.000000");
  }

  {
    auto v = runtime->evaluateStaticExpression("10.0 - 5");
    EXPECT_EQ(v.toString(), "5.000000");
  }
});
