/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"

namespace stx {

class Term {
public:

  /**
   * Reads a "yes/no" response from STDIN. Accepted responses:
   *   - Y/y -> true
   *   - N/n -> false
   *   - everything else -> RAISE exception
   */
  static bool readConfirmation();

  static char readChar();

};

} // namespace stx
