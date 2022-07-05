# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if(NOT TARGET Qt::qsb)
    return()
endif()

# Special case: test includes the QBaselineTest module sources from qtbase directly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../qtbase/tests/baseline/shared")
    add_subdirectory(scenegraph)
    add_subdirectory(controls)
endif()
