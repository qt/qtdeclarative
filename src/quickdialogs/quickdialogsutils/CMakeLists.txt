# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This library exists because QuickDialogs2 and QuickDialogs2QuickImpl
# both need QQuickNameFilter. QQuickNameFilter was originally in
# QuickDialogs2. Since QuickDialogs2 already links to
# QuickDialogs2QuickImpl, making the latter link to the former (to get
# access to QQuickNameFilter) would result in a circular dependency,
# so we have this library as a result.

qt_internal_add_module(QuickDialogs2Utils
    SOURCES
        qquickdialogtype_p.h
        qquickfilenamefilter.cpp
        qquickfilenamefilter_p.h
        qtquickdialogs2utilsglobal_p.h
    DEFINES
        QT_BUILD_QUICKDIALOGS2UTILS_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
