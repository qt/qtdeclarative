# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (QT_NO_FIND_QMLSC)
    return()
endif()

set(QT_NO_FIND_QMLSC TRUE)

# FIXME: Make this work with cross-builds
find_package(Qt6QmlCompilerPlusPrivate QUIET)
