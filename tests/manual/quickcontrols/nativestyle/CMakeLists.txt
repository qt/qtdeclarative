# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from desktopgallery.pro.
# special case skip regeneration
# Remove the skip, after somebody figures out if this is supposed
# to be a manual test, or a full-blown project like it is now
# because it was copied over from an example.

cmake_minimum_required(VERSION 3.16)
project(desktopgallery LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/desktopgallery")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(desktopgallery
    main.cpp
)
target_link_libraries(desktopgallery PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)


# Resources:
set(qmake_immediate_resource_files
    "Buttons.qml"
    "CheckBoxes.qml"
    "ComboBoxes.qml"
    "ControlContainer.qml"
    "CustomButtons.qml"
    "CustomCheckBoxes.qml"
    "CustomComboBoxes.qml"
    "CustomDials.qml"
    "CustomFrames.qml"
    "CustomProgressBars.qml"
    "CustomRadioButtons.qml"
    "CustomScrollBars.qml"
    "CustomSliders.qml"
    "CustomSpinBoxes.qml"
    "CustomTextAreas.qml"
    "CustomTextFields.qml"
    "Dials.qml"
    "Frames.qml"
    "ProgressBars.qml"
    "RadioButtons.qml"
    "ScrollBars.qml"
    "Sliders.qml"
    "SlidersMini.qml"
    "SlidersSmall.qml"
    "SpinBoxes.qml"
    "TextAreas.qml"
    "TextFields.qml"
    "checkbox-icon.png"
    "checkbox-icon16.png"
    "checkbox-icon@2x.png"
    "main.qml"
)

qt6_add_resources(desktopgallery "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS desktopgallery
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
