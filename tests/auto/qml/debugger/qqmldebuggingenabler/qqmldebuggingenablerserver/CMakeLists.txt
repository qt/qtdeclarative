# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qqmldebuggingenablerserver.pro.

#####################################################################
## qqmldebuggingenablerserver Binary:
#####################################################################

qt_internal_add_executable(qqmldebuggingenablerserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../qqmldebuggingenabler"
    # special case begin
    # Remove INSTALL_DIRECTORY with $$[QT_INSTALL_TESTS]
    # as these are invalid
    # INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qqmldebuggingenabler"
    # special case end
    SOURCES
        qqmldebuggingenablerserver.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

## Scopes:
#####################################################################
