# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlitemmodels.pro.

#####################################################################
## tst_qqmlitemmodels Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlitemmodels
    SOURCES
        qtestmodel.h
        testtypes.h
        tst_qqmlitemmodels.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlitemmodels.pro:<TRUE>:
# DISTFILES = "data/modelindex.qml" "data/persistentmodelindex.qml" "data/itemselectionrange.qml" "data/modelindexlist.qml" "data/itemselection.qml" "data/modelindexconversion.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlitemmodels CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlitemmodels CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
