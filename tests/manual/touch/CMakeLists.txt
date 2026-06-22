# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from touch.pro.

#####################################################################
## touch Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_touch
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
    "basic-flickable.qml"
    "flickresize.qml"
    "flicktext.qml"
    "main.qml"
    "mpta-crosshairs.qml"
    "qt-logo.jpg"
    "bearwhack/heart-blur.png"
    "bearwhack/Bear0.png"
    "bearwhack/BearWhackParticleSystem.qml"
    "bearwhack/blur-circle.png"
    "bearwhack/Bear1.png"
    "bearwhack/bearwhack.qml"
    "bearwhack/Bear2.png"
    "bearwhack/title.png"
    "bearwhack/blur-circle3.png"
    "bearwhack/Bear3.png"
    "bearwhack/AugmentedTouchPoint.qml"
    "bearwhack/BearB.png"
)

qt_internal_add_resource(tst_manual_touch "qml"
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

qt_internal_add_resource(tst_manual_touch "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../shared"
    FILES
        ${quick_shared_resource_files}
)
