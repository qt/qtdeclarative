# Generated from qjstest.pro.

#####################################################################
## qjstest Tool:
#####################################################################

add_qt_tool(qjstest
    SOURCES
        main.cpp
        test262runner.cpp test262runner.h
    DEFINES
        QT_DEPRECATED_WARNINGS
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qjstest.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Javascript" "test" "runner"
# TEMPLATE = "app"
# _LOADED = "qt_tool"
