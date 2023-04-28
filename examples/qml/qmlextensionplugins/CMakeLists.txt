# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qmlqtimeexample LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/qmlextensionplugins/imports/TimeExample")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_standard_project_setup(REQUIRES 6.5)

#![0]
set(qml_files
    imports/TimeExample/Clock.qml
)

set(images
    imports/TimeExample/center.png
    imports/TimeExample/clock.png
    imports/TimeExample/hour.png
    imports/TimeExample/minute.png
)
#![0]

foreach(file IN LISTS qml_files images)
    get_filename_component(filename ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${filename})
endforeach()

#![1]
qt_add_qml_module(qmlqtimeexample
    OUTPUT_DIRECTORY imports/TimeExample
    URI "TimeExample"
    SOURCES timemodel.cpp timemodel.h
    QML_FILES ${qml_files}
    RESOURCES ${images}
)
#![1]

set_target_properties(qmlqtimeexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qmlqtimeexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
)

install(TARGETS qmlqtimeexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
