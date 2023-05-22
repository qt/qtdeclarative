# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QmlLSPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qlspcustomtypes_p.h
        qlanguageserver_p.h qlanguageserver_p.h qlanguageserver.cpp
        qqmllanguageserver_p.h qqmllanguageserver.cpp
        qworkspace.cpp qworkspace_p.h
        qtextblock_p.h qtextblock.cpp
        qtextcursor_p.h qtextcursor.cpp
        qtextdocument.cpp qtextdocument_p.h
        qqmllintsuggestions_p.h qqmllintsuggestions.cpp
        qtextsynchronization.cpp qtextsynchronization_p.h
        qqmlcompletionsupport_p.h qqmlcompletionsupport.cpp
        qqmlcodemodel_p.h qqmlcodemodel.cpp
        qqmlbasemodule_p.h
        qqmlgototypedefinitionsupport_p.h qqmlgototypedefinitionsupport.cpp
        qqmlformatting_p.h qqmlformatting.cpp
        qqmllsutils_p.h qqmllsutils.cpp
        qqmlfindusagessupport_p.h qqmlfindusagessupport.cpp
        qqmlgotodefinitionsupport.cpp qqmlgotodefinitionsupport_p.h

    PUBLIC_LIBRARIES
        Qt::LanguageServerPrivate
        Qt::CorePrivate
        Qt::QmlDomPrivate
        Qt::QmlCompilerPrivate
        Qt::QmlToolingSettingsPrivate
        Qt::LanguageServerPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
