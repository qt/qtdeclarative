# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.19)

project(test_internal_singleton)

find_package(Qt6 REQUIRED COMPONENTS Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(test)

set_target_properties(all_qmllint PROPERTIES EXCLUDE_FROM_ALL FALSE)

