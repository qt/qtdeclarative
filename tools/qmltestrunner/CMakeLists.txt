# Generated from qmltestrunner.pro.

#####################################################################
## qmltestrunner Tool:
#####################################################################

add_qt_tool(qmltestrunner
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickTest
)

#### Keys ignored in scope 1:.:.:qmltestrunner.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Test" "Runner"
