# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlLintPrivate
    PLUGIN_TYPES qmllint
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmljscontextproperties_p.h qqmljscontextproperties.cpp
        qqmljslinter_p.h qqmljslinter.cpp
        qqmljslintercodegen_p.h qqmljslintercodegen.cpp
        qqmljslinterpasses.cpp qqmljslinterpasses_p.h
        qqmljslintertypepropagator_p.h qqmljslintertypepropagator.cpp
        qqmljslintervisitor_p.h qqmljslintervisitor.cpp
        qqmljsliteralbindingcheck_p.h qqmljsliteralbindingcheck.cpp
        qqmljsvaluetypefromstringcheck_p.h qqmljsvaluetypefromstringcheck.cpp
        qqmljsusercontextproperties_p.h qqmljsusercontextproperties.cpp
        qqmljslinterrenamedcomponents_p.h qqmljslinterrenamedcomponents.cpp
        qqmljslintercontext_p.h

    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlCompilerPrivate
        Qt::QmlToolingSettingsPrivate
)
