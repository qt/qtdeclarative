// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// SPDX-License-Identifier: MIT

#include "Utils.h"
#include <stdexcept>

QT_YOGA_NAMESPACE_BEGIN

using namespace facebook;

YGFlexDirection YGFlexDirectionCross(
    const YGFlexDirection flexDirection,
    const YGDirection direction) {
  return YGFlexDirectionIsColumn(flexDirection)
      ? YGResolveFlexDirection(YGFlexDirectionRow, direction)
      : YGFlexDirectionColumn;
}

float YGFloatMax(const float a, const float b) {
  if (!yoga::isUndefined(a) && !yoga::isUndefined(b)) {
    return fmaxf(a, b);
  }
  return yoga::isUndefined(a) ? b : a;
}

float YGFloatMin(const float a, const float b) {
  if (!yoga::isUndefined(a) && !yoga::isUndefined(b)) {
    return fminf(a, b);
  }

  return yoga::isUndefined(a) ? b : a;
}

bool YGValueEqual(const YGValue& a, const YGValue& b) {
  if (a.unit != b.unit) {
    return false;
  }

  if (a.unit == YGUnitUndefined ||
      (yoga::isUndefined(a.value) && yoga::isUndefined(b.value))) {
    return true;
  }

  return fabs(a.value - b.value) < 0.0001f;
}

bool YGFloatsEqual(const float a, const float b) {
  if (!yoga::isUndefined(a) && !yoga::isUndefined(b)) {
    return fabs(a - b) < 0.0001f;
  }
  return yoga::isUndefined(a) && yoga::isUndefined(b);
}

bool YGDoubleEqual(const double a, const double b) {
  if (!yoga::isUndefined(a) && !yoga::isUndefined(b)) {
    return fabs(a - b) < 0.0001;
  }
  return yoga::isUndefined(a) && yoga::isUndefined(b);
}

float YGFloatSanitize(const float val) {
  return yoga::isUndefined(val) ? 0 : val;
}

YGFloatOptional YGFloatOptionalMax(YGFloatOptional op1, YGFloatOptional op2) {
  if (op1 >= op2) {
    return op1;
  }
  if (op2 > op1) {
    return op2;
  }
  return op1.isUndefined() ? op2 : op1;
}

void yoga::throwLogicalErrorWithMessage([[maybe_unused]]const char* message) {
#if defined(__cpp_exceptions)
  throw std::logic_error(message);
#else // !defined(__cpp_exceptions)
  std::terminate();
#endif // defined(__cpp_exceptions)
}

QT_YOGA_NAMESPACE_END
