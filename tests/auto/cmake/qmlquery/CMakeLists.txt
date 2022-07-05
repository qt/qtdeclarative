# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

cmake_minimum_required(VERSION 3.16...3.22)
project(qmlquery)

find_package(Qt6 REQUIRED COMPONENTS Qml)
set(CMAKE_AUTOMOC TRUE)

add_subdirectory(My/Things)
