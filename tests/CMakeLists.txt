# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from tests.pro.

if(QT_BUILD_STANDALONE_TESTS)
    # Add qt_find_package calls for extra dependencies that need to be found when building
    # the standalone tests here.
    qt_find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS QmlLSPrivate)
endif()
qt_build_tests()
