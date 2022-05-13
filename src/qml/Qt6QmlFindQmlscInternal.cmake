# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if (QT_NO_FIND_QMLSC)
    return()
endif()

set(QT_NO_FIND_QMLSC TRUE)
if(NOT "${QT_HOST_PATH}" STREQUAL "")
    # Make sure that the tools find the host tools and does not try the target
    set(BACKUP_qmlsc_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(BACKUP_qmlsc_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
    set(BACKUP_qmlsc_CMAKE_SYSROOT ${CMAKE_SYSROOT})
    set(CMAKE_PREFIX_PATH "${QT_HOST_PATH_CMAKE_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${_qt_additional_host_packages_prefix_paths}")
    set(CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${_qt_additional_host_packages_root_paths}")
    unset(CMAKE_SYSROOT)
endif()

find_package(Qt6QmlCompilerPlusPrivate QUIET)

if(NOT "${QT_HOST_PATH}" STREQUAL "")
    set(CMAKE_PREFIX_PATH ${BACKUP_qmlsc_CMAKE_PREFIX_PATH})
    set(CMAKE_FIND_ROOT_PATH ${BACKUP_qmlsc_CMAKE_FIND_ROOT_PATH})
    set(CMAKE_SYSROOT ${BACKUP_qmlsc_CMAKE_SYSROOT})
endif()

