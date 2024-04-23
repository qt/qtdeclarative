# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (TARGET Qt::QmlLSPrivate)
    add_subdirectory(lifecycle)
    add_subdirectory(utils)
    add_subdirectory(qqmlcodemodel)
    add_subdirectory(qmlls)
    add_subdirectory(modules)
    add_subdirectory(cli)
endif()

