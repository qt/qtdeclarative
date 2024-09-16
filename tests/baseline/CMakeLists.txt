# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt::qsb)
    return()
endif()

# Special case: test includes the QBaselineTest module sources from qtbase directly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../qtbase/tests/baseline/shared")
    if(QT_FEATURE_processenvironment AND QT_FEATURE_process)
        add_subdirectory(scenegraph)
        add_subdirectory(controls)
    endif()
endif()
