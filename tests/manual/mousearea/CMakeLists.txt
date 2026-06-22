# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from mousearea.pro.

#####################################################################
## mousearea Binary:
#####################################################################

qt_internal_add_manual_test(mousearea
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
    "plainMouseArea.qml"
)

qt_internal_add_resource(mousearea "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(quick_shared_resource_files
    "../shared/Button.qml"
    "../shared/CheckBox.qml"
    "../shared/Label.qml"
    "../shared/LauncherList.qml"
    "../shared/SimpleLauncherDelegate.qml"
    "../shared/Slider.qml"
    "../shared/TextField.qml"
    "../shared/images/back.png"
    "../shared/images/checkmark.png"
    "../shared/images/next.png"
    "../shared/images/slider_handle.png"
)

qt_internal_add_resource(mousearea "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../shared"
    FILES
        ${quick_shared_resource_files}
)


#### Keys ignored in scope 1:.:.:mousearea.pro:<TRUE>:
# TEMPLATE = "app"
