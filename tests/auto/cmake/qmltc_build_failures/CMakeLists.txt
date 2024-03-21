# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qmltc_build_failures)

_qt_internal_test_expect_build_fail(nontoplevelrequiredproperty)
