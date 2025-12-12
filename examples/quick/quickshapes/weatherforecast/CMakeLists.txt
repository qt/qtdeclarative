# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(weatherforecast LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickTools)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../../shared" "shared")

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
    "BouncyShape.qml"
    "Cloud.qml"
    "CloudWithLightning.qml"
    "CloudWithRain.qml"
    "CloudWithSnow.qml"
    "Gear.qml"
    "MapLabel.qml"
    "SettingsDrawer.qml"
    "Sun.qml"
    "SunBehindCloud.qml"
    "SunBehindRainCloud.qml"
    "SunBehindSmallCloud.qml"
    "DemoShape.qml"
  RESOURCES
    "assets/WorkSans-Regular.ttf"
)

qt_target_qml_from_svg(weatherforecastexample
    OPTIMIZE_PATHS
    TYPE_NAME "DemoShape"
    COPYRIGHT_STATEMENT
        "Copyright © 2024 The Qt Company Ltd. Copyright © 2010 Kolja21\\nSPDX-License-Identifier: CC-BY-3.0"
    FILES
        assets/Europe.svg
    OUTPUTS
        Europe_generated.qml
)

#![svgtoqml]
qt_target_qml_from_svg(weatherforecastexample
    OPTIMIZE_PATHS
    TYPE_NAME "DemoShape"
    COPYRIGHT_STATEMENT "Copyright © 2023 Framework7\\nSPDX-License-Identifier: MIT"
    FILES
        assets/gear-alt-stroke.svg
    OUTPUTS
        Gear_generated.qml
)
#![svgtoqml]

qt_target_qml_from_svg(weatherforecastexample
    OPTIMIZE_PATHS
    TYPE_NAME "DemoShape"
    COPYRIGHT_STATEMENT "Copyright © 2023 Googlefonts\\nSPDX-License-Identifier: Apache-2.0"
    FILES
        assets/cloud-svgrepo-com.svg
        assets/cloud-with-lightning-svgrepo-com.svg
        assets/cloud-with-lightning-and-rain-svgrepo-com.svg
        assets/cloud-with-rain-svgrepo-com.svg
        assets/cloud-with-snow-svgrepo-com.svg
        assets/sun-svgrepo-com.svg
        assets/sun-behind-large-cloud-svgrepo-com.svg
        assets/sun-behind-rain-cloud-svgrepo-com.svg
        assets/sun-behind-small-cloud-svgrepo-com.svg
        assets/sun-behind-cloud-svgrepo-com.svg
    OUTPUTS
        Cloud_generated.qml
        CloudWithLightning_generated.qml
        CloudWithLightningAndRain_generated.qml
        CloudWithRain_generated.qml
        CloudWithSnow_generated.qml
        Sun_generated.qml
        SunBehindLargeCloud_generated.qml
        SunBehindRainCloud_generated.qml
        SunBehindSmallCloud_generated.qml
        SunBehindCloud_generated.qml
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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET weatherforecastexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
