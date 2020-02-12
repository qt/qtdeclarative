# Generated from qquicksystempalette.pro.

#####################################################################
## tst_qquicksystempalette Test:
#####################################################################

qt_add_test(tst_qquicksystempalette
    SOURCES
        tst_qquicksystempalette.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_qquicksystempalette CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)
