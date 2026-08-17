# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(parse LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Svg Qml Quick)
find_package(Qt6 COMPONENTS GuiPrivate QuickVectorImagePrivate QuickPrivate QuickVectorImageGeneratorPrivate)

qt_add_executable(qquickvectorimage
    main.cpp
)

target_link_libraries(qquickvectorimage PUBLIC
    Qt::Core
    Qt::GuiPrivate
    Qt::Svg
    Qt::Qml
    Qt::Quick
    Qt::QuickVectorImagePrivate
    Qt::QuickPrivate
    Qt::QuickVectorImageGeneratorPrivate
)
target_link_libraries(qquickvectorimage PRIVATE
     -fsanitize=fuzzer
)

