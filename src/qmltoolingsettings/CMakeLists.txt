# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlToolingSettingsPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmltoolingsettings_p.h
        qqmltoolingsettings.cpp

        qqmltoolingutils_p.h
        qqmltoolingutils.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)
