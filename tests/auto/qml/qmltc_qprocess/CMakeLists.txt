# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qmltc_qprocess
    SOURCES
        tst_qmltc_qprocess.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QuickTestUtilsPrivate
)

# special setup for singleton files:
set_source_files_properties(data/SingletonThing.qml data/singletonUncreatable.qml
    PROPERTIES QT_QML_SINGLETON_TYPE true)

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(tst_qmltc_qprocess
    VERSION 1.0
    URI QmltcQProcessTests
    SOURCES
        cpptypes/testtype.h
    DEPENDENCIES
        QtQuick/auto
    QML_FILES
        data/dummy.qml
        data/inlineComponentInvalidAlias.qml
        data/SingletonThing.qml
        data/erroneousFile.qml
        data/invalidAliasRevision.qml
        data/ComponentType.qml
        data/inlineComponentWithEnum.qml
        data/singletonUncreatable.qml
        data/uncreatable.qml
        data/invalidSignalHandlers.qml
)

set(common_libraries
    Qt::Core
    Qt::QmlPrivate
    Qt::QuickPrivate
    Qt::TestPrivate
    Qt::Gui # QColor, QMatrix4x4, ...
)

# use PUBLIC everywhere to simplify the build of the test binary
target_include_directories(tst_qmltc_qprocess PUBLIC cpptypes/)
target_link_libraries(tst_qmltc_qprocess PUBLIC ${common_libraries})

add_dependencies(tst_qmltc_qprocess Qt::qmltc)

# fetch --resource arguments manually (mimics the logic of qmltc compilation
# command)
_qt_internal_genex_getjoinedproperty(qrc_args tst_qmltc_qprocess
    _qt_generated_qrc_files "" "_::_"
)
target_compile_definitions(tst_qmltc_qprocess PRIVATE
    TST_QMLTC_QPROCESS_RESOURCES="${qrc_args}"
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
