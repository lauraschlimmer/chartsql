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
#include <fnord/ieee754.h>
#include <fnord/util/binarymessagereader.h>
#include <fnord/util/binarymessagewriter.h>
#include <chartsql/svalue.h>

using namespace fnord;

namespace csql {

enum kFunctionType {
  FN_PURE,
  FN_AGGREGATE
};

/**
 * A pure/stateless expression that returns a single return value
 */
struct PureFunction {
  void (*call)(int argc, SValue* in, SValue* out);
};

/**
 * An aggregate expression that returns a single return value
 */
struct AggregateFunction {
  size_t scratch_size;
  void (*accumulate)(void* scratch, int argc, SValue* in);
  void (*get)(void* scratch, SValue* out);
  void (*reset)(void* scratch);
  void (*init)(void* scratch);
  void (*free)(void* scratch);
};

struct SFunction {
  kFunctionType type;
  union {
    PureFunction t_pure;
    AggregateFunction t_aggregate;
  } vtable;
};


}
