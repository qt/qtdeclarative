# Generated from qjsvalue.pro.

#####################################################################
## tst_qjsvalue Test:
#####################################################################

qt_find_package(Threads PROVIDED_TARGETS Threads::Threads) # special case

qt_add_test(tst_qjsvalue
    SOURCES
        tst_qjsvalue.cpp tst_qjsvalue.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Widgets
    LIBRARIES # special case
        Threads::Threads # special case
)

## Scopes:
#####################################################################
