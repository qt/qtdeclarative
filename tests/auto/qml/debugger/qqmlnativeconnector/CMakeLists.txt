# Generated from qqmlnativeconnector.pro.

#####################################################################
## tst_qqmlnativeconnector Test:
#####################################################################

add_qt_test(tst_qqmlnativeconnector
    GUI
    SOURCES
        tst_qqmlnativeconnector.cpp
    DEFINES
        QT_QML_DEBUG
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

## Scopes:
#####################################################################
