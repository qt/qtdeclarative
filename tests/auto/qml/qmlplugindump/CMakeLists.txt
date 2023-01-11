# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlplugindump.pro.

#####################################################################
## tst_qmlplugindump Test:
#####################################################################

qt_internal_add_test(tst_qmlplugindump
    SOURCES
        tst_qmlplugindump.cpp
    DEFINES
        QT_QMLTEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickTestUtilsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmlplugindump CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmlplugindump CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
add_subdirectory(data/dumper/Dummy)
add_subdirectory(data/dumper/Imports)
add_subdirectory(data/dumper/Versions)
add_subdirectory(data/dumper/ExtendedType)
