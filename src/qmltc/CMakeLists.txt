# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmltcPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmltccodewriter.cpp qqmltccodewriter_p.h
        qqmltccommandlineutils.cpp qqmltccommandlineutils_p.h
        qqmltccompiler.cpp qqmltccompiler_p.h
        qqmltccompilerpieces.cpp qqmltccompilerpieces_p.h
        qqmltcoutputir_p.h
        qqmltcoutputprimitives_p.h
        qqmltcpropertyutils_p.h
        qqmltctyperesolver.cpp qqmltctyperesolver_p.h
        qqmltcvisitor.cpp qqmltcvisitor_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
        Qt::QmlLintPrivate
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
)
