# Generated from nodes.pro.

#####################################################################
## tst_nodestest Test:
#####################################################################

add_qt_test(tst_nodestest
    GUI
    SOURCES
        tst_nodestest.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:nodes.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:nodes.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
