/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <yoga/YGEnums.h>
#include <yoga/YGMacros.h>

QT_YOGA_NAMESPACE_BEGIN 

YG_EXTERN_C_BEGIN

typedef struct YGValue {
  float value;
  YGUnit unit;
} YGValue;

YOGA_EXPORT extern const YGValue YGValueAuto;
YOGA_EXPORT extern const YGValue YGValueUndefined;
YOGA_EXPORT extern const YGValue YGValueZero;

YG_EXTERN_C_END

#ifdef __cplusplus
#include <limits>
constexpr float YGUndefined = std::numeric_limits<float>::quiet_NaN();
#else
#include <math.h>
#define YGUndefined NAN
#endif

#ifdef __cplusplus
inline bool operator==(const YGValue& lhs, const YGValue& rhs) {
  if (lhs.unit != rhs.unit) {
    return false;
  }

  switch (lhs.unit) {
    case YGUnitUndefined:
    case YGUnitAuto:
      return true;
    case YGUnitPoint:
    case YGUnitPercent:
      return lhs.value == rhs.value;
  }

  return false;
}

inline bool operator!=(const YGValue& lhs, const YGValue& rhs) {
  return !(lhs == rhs);
}

inline YGValue operator-(const YGValue& value) {
  return {-value.value, value.unit};
}
#endif

QT_YOGA_NAMESPACE_END
