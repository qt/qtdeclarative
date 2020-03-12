# Generated from qmlformat.pro.

#####################################################################
## qmlformat Tool:
#####################################################################

qt_add_tool(qmlformat
    TOOLS_TARGET Qml # special case
    SOURCES
        commentastvisitor.cpp commentastvisitor.h
        dumpastvisitor.cpp dumpastvisitor.h
        main.cpp
        restructureastvisitor.cpp restructureastvisitor.h
    PUBLIC_LIBRARIES
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlformat.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Formatter"
# _OPTION = "host_build"
