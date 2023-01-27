# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (TARGET Qt::LanguageServerPrivate AND TARGET Qt::qmlls)
    add_subdirectory(qmlls)
    add_subdirectory(lifecycle)
    add_subdirectory(modules)
    add_subdirectory(utils)
endif()
