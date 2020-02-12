# Generated from qjstest.pro.

#####################################################################
## qjstest Tool:
#####################################################################

qt_add_tool(qjstest
    SOURCES
        main.cpp
        test262runner.cpp test262runner.h
    DEFINES
        QT_DEPRECATED_WARNINGS
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qjstest.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Javascript" "test" "runner"
# TEMPLATE = "app"
