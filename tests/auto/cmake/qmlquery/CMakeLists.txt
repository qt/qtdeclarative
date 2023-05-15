# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...3.22)
project(qmlquery)

find_package(Qt6 REQUIRED COMPONENTS Qml)
set(CMAKE_AUTOMOC TRUE)

add_subdirectory(My/Things)
add_subdirectory(My/OtherThings)
