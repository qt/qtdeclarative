#####################################################################
## qmlls Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlls)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML languageserver"
    TOOLS_TARGET Qml # special case
    SOURCES
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
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::CorePrivate
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()
