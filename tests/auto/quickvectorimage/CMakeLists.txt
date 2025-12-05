# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Limit set of tests to run for static Qt builds.
if(QT_BUILD_MINIMAL_STATIC_TESTS)
    return()
endif()

add_subdirectory(qquickvectorimage)
