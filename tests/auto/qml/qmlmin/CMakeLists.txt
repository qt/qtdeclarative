# Generated from qmlmin.pro.

#####################################################################
## tst_qmlmin Test:
#####################################################################

add_qt_test(tst_qmlmin
    SOURCES
        tst_qmlmin.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

## Scopes:
#####################################################################

extend_target(tst_qmlmin CONDITION cross_compile AND NOT boot2qt
    DEFINES
        QTEST_CROSS_COMPILED
)
