# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmldebugjsserver.pro.

#####################################################################
## qqmldebugjsserver Binary:
#####################################################################

qt_internal_add_executable(qqmldebugjsserver
    # special case begin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    # INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/qqmldebugjsserver"
    # special case end
    SOURCES
        qqmldebugjsserver.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

## Scopes:
#####################################################################
