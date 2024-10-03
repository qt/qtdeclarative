# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearableexample VERSION 1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)
find_package(Qt6 QUIET OPTIONAL_COMPONENTS Positioning Location)

qt_standard_project_setup()

add_subdirectory(Wearable)
add_subdirectory(WearableSettings)
add_subdirectory(WearableStyle)

qt_add_executable(wearableexample WIN32 MACOSX_BUNDLE
    wearable.cpp
)

target_link_libraries(wearableexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(TARGET Qt6::Positioning)
    target_link_libraries(wearableexample PUBLIC
        Qt6::Positioning
    )
endif()

if(TARGET Qt6::Location)
    target_link_libraries(wearableexample PUBLIC
        Qt6::Location
    )
endif()

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(wearableexample PRIVATE Qt6::QuickTemplates2)
endif()

set_target_properties(wearableexample PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.examples.wearable"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(TARGETS wearableexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET wearableexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

