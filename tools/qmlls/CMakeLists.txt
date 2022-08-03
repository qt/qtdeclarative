#####################################################################
## qmlls Tool:
#####################################################################

qt_internal_add_app(qmlls
    TARGET_DESCRIPTION "QML Language Server"
    SOURCES
        qlanguageserver.h qlanguageserver_p.h qlanguageserver.cpp
        qqmllanguageserver.h qqmllanguageserver.cpp
        qmllanguageservertool.cpp
        textblock.h textblock.cpp
        textcursor.h textcursor.cpp
        textcursor.cpp textcursor.h
        textdocument.cpp textdocument.h
        qmllintsuggestions.h qmllintsuggestions.cpp
        textsynchronization.cpp textsynchronization.h
        qqmlcodemodel.h qqmlcodemodel.cpp
        ../shared/qqmltoolingsettings.h
        ../shared/qqmltoolingsettings.cpp
    DEFINES
        QT_USE_QSTRINGBUILDER
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::CorePrivate
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::QmlLintPrivate
)
set_target_properties(qmlls PROPERTIES WIN32_EXECUTABLE FALSE)
