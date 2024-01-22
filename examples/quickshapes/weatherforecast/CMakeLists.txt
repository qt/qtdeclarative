# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(weatherforecast LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickshapes/weatherforecast")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../../quick/shared" "shared")

qt_add_executable(weatherforecastexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(weatherforecastexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(weatherforecastexample weatherforecast_shared)

qt_add_qml_module(weatherforecastexample
  URI weatherforecast
  QML_FILES
    "main.qml"
    "Europe.qml"
    "Europe_generated.qml"
    "BouncyShape.qml"
    "Cloud.qml"
    "Cloud_generated.qml"
    "CloudWithLightning.qml"
    "CloudWithLightning_generated.qml"
    "CloudWithRain.qml"
    "CloudWithRain_generated.qml"
    "CloudWithSnow.qml"
    "CloudWithSnow_generated.qml"
    "Gear.qml"
    "Gear_generated.qml"
    "MapLabel.qml"
    "SettingsDrawer.qml"
    "Sun.qml"
    "Sun_generated.qml"
    "SunBehindCloud.qml"
    "SunBehindCloud_generated.qml"
    "SunBehindRainCloud.qml"
    "SunBehindRainCloud_generated.qml"
    "SunBehindSmallCloud.qml"
    "SunBehindSmallCloud_generated.qml"
    "DemoShape.qml"
  RESOURCES
    "assets/WorkSans-Regular.ttf"
)

set(resource_files
    "qtquickcontrols2.conf"
)

qt_add_resources(weatherforecastexample "res"
    PREFIX
        "/"
    FILES
        ${resource_files}
)

install(TARGETS weatherforecastexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(weatherforecastexample)
