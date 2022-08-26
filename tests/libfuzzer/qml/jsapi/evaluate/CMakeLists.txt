# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(evaluate LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(evaluate
    main.cpp
)

target_link_libraries(evaluate PUBLIC
    Qt::Core
    Qt::Quick
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(evaluate PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(evaluate PRIVATE
        -fsanitize=fuzzer
    )
endif()

qt_import_plugins(evaluate
    INCLUDE Qt::QMinimalIntegrationPlugin
)
