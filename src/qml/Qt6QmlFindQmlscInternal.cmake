# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if (QT_NO_FIND_QMLSC)
    return()
endif()

set(QT_NO_FIND_QMLSC TRUE)

# FIXME: Make this work with cross-builds
find_package(Qt6QmlCompilerPlusPrivate QUIET)
