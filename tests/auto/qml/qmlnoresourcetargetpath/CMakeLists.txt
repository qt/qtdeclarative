# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlnoresourcetargetpath LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmlnoresourcetargetpath
    SOURCES
        tst_qmlnoresourcetargetpath.cpp
    LIBRARIES
        Qt::Qml
        ntp_versioned_module
        ntp_versioned_moduleplugin
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

add_subdirectory(NtpVersioned)

# NO_RESOURCE_TARGET_PATH puts the QML files and the qmldir at qrc:/ instead of
# qrc:/qt/qml/NoResourceTargetPath/.
qt_add_qml_module(tst_qmlnoresourcetargetpath
    URI NoResourceTargetPath
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        NtpBurner.qml
        NtpMixedVersions.qml
        NtpQualifiedOnly.qml
        NtpShifting1.qml
        NtpShifting2.qml
        NtpThing.qml
    NO_LINT
    __QT_INTERNAL_NO_GENERATE_QMLLS_BUILD_INI
)
