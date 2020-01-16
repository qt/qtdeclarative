# Generated from qmlmin.pro.

#####################################################################
## tst_qmlmin Test:
#####################################################################

qt_add_test(tst_qmlmin
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

qt_extend_target(tst_qmlmin CONDITION CMAKE_CROSSCOMPILING AND NOT boot2qt
    DEFINES
        QTEST_CROSS_COMPILED
)
