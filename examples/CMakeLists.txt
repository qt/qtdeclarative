# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(qml)
if(TARGET Qt6::Quick)
    add_subdirectory(quick)
    add_subdirectory(platforms)
endif()
if(TARGET Qt6::QuickTemplates2)
    add_subdirectory(quickcontrols)
endif()

qt_examples_build_end()
