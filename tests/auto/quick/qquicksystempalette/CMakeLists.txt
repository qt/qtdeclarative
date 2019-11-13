# Generated from qquicksystempalette.pro.

#####################################################################
## tst_qquicksystempalette Test:
#####################################################################

add_qt_test(tst_qquicksystempalette
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

## Scopes:
#####################################################################

extend_target(tst_qquicksystempalette CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
