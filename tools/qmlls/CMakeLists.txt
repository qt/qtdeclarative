# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmlls Tool:
#####################################################################

qt_internal_add_app(qmlls
    TARGET_DESCRIPTION "QML Language Server"
    SOURCES
        lspcustomtypes.h
        qlanguageserver.h qlanguageserver_p.h qlanguageserver.cpp
        qqmllanguageserver.h qqmllanguageserver.cpp
        qmllanguageservertool.cpp
        workspace.cpp workspace.h
        textblock.h textblock.cpp
        textcursor.h textcursor.cpp
        textcursor.cpp textcursor.h
        textdocument.cpp textdocument.h
        qmllintsuggestions.h qmllintsuggestions.cpp
        textsynchronization.cpp textsynchronization.h
        qmlcompletionsupport.h qmlcompletionsupport.cpp
        qqmlcodemodel.h qqmlcodemodel.cpp
        ../shared/qqmltoolingsettings.h
        ../shared/qqmltoolingsettings.cpp
    DEFINES
        QT_USE_QSTRINGBUILDER
    LIBRARIES
        Qt::QmlPrivate
        Qt::CorePrivate
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::QmlCompilerPrivate
)
set_target_properties(qmlls PROPERTIES WIN32_EXECUTABLE FALSE)
