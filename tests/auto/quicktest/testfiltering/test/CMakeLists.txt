# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from test.pro.

#####################################################################
## tst_testfiltering Test:
#####################################################################

qt_internal_add_test(tst_testfiltering
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_testfiltering.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)
