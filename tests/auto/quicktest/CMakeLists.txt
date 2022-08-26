# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quicktest.pro.

add_subdirectory(polish)
add_subdirectory(polish-qml)
add_subdirectory(signalspy)
add_subdirectory(quicktestmainwithsetup)
if(QT_FEATURE_process AND NOT ANDROID) # QTBUG-100175
    add_subdirectory(testfiltering)
endif()
add_subdirectory(testswithcomponents)
