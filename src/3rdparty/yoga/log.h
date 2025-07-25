/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <yoga/YGEnums.h>

QT_YOGA_NAMESPACE_BEGIN

struct YGNode;
struct YGConfig;
namespace facebook {
namespace yoga {

namespace detail {

struct Log {
  static void log(
      YGNode* node,
      YGLogLevel level,
      void*,
      const char* message,
      ...) noexcept;

  static void log(
      YGConfig* config,
      YGLogLevel level,
      void*,
      const char* format,
      ...) noexcept;
};

} // namespace detail
} // namespace yoga
} // namespace facebook

QT_YOGA_NAMESPACE_END
