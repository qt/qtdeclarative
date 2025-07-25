/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef DEBUG

#pragma once

#include <string>

#include <yoga/Yoga.h>

QT_YOGA_NAMESPACE_BEGIN

namespace facebook {
namespace yoga {

void YGNodeToString(
    std::string& str,
    YGNodeRef node,
    YGPrintOptions options,
    uint32_t level);

} // namespace yoga
} // namespace facebook

QT_YOGA_NAMESPACE_END

#endif
