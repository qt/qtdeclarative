# Generated from qquicksystempalette.pro.

#####################################################################
## tst_qquicksystempalette Test:
#####################################################################

add_qt_test(tst_qquicksystempalette
    GUI
    SOURCES
        tst_qquicksystempalette.cpp
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

#### Keys ignored in scope 1:.:.:qquicksystempalette.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquicksystempalette.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquicksystempalette CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
