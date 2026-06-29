# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml Core)

qt_standard_project_setup(REQUIRES 6.8)

include(${RunCMake_TEST}.cmake)
