# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (TARGET Qt::LanguageServerPrivate AND TARGET Qt::qmlls)
    add_subdirectory(lifecycle)
    if (TARGET Qt::QuickTestUtilsPrivate)
        add_subdirectory(qmlls)
        add_subdirectory(utils)
        add_subdirectory(modules)
        add_subdirectory(qqmlcodemodel)
    endif()
endif()
