# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_how-to-qml
    SOURCES
        tst_how-to-qml.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickTemplates2Private
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_how-to-qml
    URI HowToQml
    QML_FILES
        active-focus-debugging/ActiveFocusDebuggingMain.qml
        time-picker/TimeComponentLabel.qml
        time-picker/TimePickerDialog.qml
        time-picker/TimePickerMain.qml
        time-picker/TimePickerLabel.qml
        time-picker/TimePicker.qml
)
