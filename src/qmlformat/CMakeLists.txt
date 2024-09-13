# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlFormatPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmlformatoptions.cpp qqmlformatoptions_p.h
        qqmlformatsettings.cpp qqmlformatsettings_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::QmlToolingSettingsPrivate
)
