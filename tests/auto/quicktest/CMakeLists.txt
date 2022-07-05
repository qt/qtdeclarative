# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from quicktest.pro.

add_subdirectory(polish)
add_subdirectory(polish-qml)
add_subdirectory(signalspy)
add_subdirectory(quicktestmainwithsetup)
if(QT_FEATURE_process AND NOT ANDROID) # QTBUG-100175
    add_subdirectory(testfiltering)
endif()
add_subdirectory(testswithcomponents)
