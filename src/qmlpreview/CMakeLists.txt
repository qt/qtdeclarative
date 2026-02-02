# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlPreviewPrivate Module:
#####################################################################

qt_internal_add_module(QmlPreviewPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmlpreviewdiff.cpp qqmlpreviewdiff_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlPrivate
)
