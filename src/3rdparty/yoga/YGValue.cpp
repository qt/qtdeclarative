// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// SPDX-License-Identifier: MIT

#include <yoga/YGValue.h>

QT_YOGA_NAMESPACE_BEGIN

const YGValue YGValueZero = {0, YGUnitPoint};
const YGValue YGValueUndefined = {YGUndefined, YGUnitUndefined};
const YGValue YGValueAuto = {YGUndefined, YGUnitAuto};

QT_YOGA_NAMESPACE_END
