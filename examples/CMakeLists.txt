# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(qml)
if(TARGET Qt::QuickTest)
    add_subdirectory(qmltest)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(quick)
endif()
if(TARGET Qt::QuickTemplates2)
    add_subdirectory(quickcontrols)
endif()

qt_examples_build_end()
