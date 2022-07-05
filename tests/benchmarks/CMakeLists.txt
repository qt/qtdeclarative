# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from benchmarks.pro.

add_subdirectory(qml)
add_subdirectory(quick)
if(QT_FEATURE_private_tests)
    add_subdirectory(particles)
endif()
add_subdirectory(quickcontrols2)
