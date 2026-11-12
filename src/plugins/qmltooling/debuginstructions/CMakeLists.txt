# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## DebugInstructionsPrivate Module:
#####################################################################

qt_internal_add_module(DebugInstructionsPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qv4debuginstructions.cpp qv4debuginstructions_p.h
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
    NO_GENERATE_CPP_EXPORTS
)
