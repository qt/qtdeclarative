# Generated from geometry.pro.

#####################################################################
## tst_geometry Test:
#####################################################################

add_qt_test(tst_geometry
    GUI
    SOURCES
        tst_geometry.cpp
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

#### Keys ignored in scope 1:.:.:geometry.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:geometry.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
