# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlToolingSettingsPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmltoolingsettings_p.h
        qqmltoolingsettings.cpp
    PUBLIC_LIBRARIES
        Qt::Core
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
