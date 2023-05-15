# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(library)

add_library(library ThirdPartyDisplay.cpp ThirdPartyDisplay.h)

qt_extract_metatypes(library)

target_link_libraries(library PUBLIC
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Gui
)
