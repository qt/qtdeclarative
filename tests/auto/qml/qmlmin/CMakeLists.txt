# Generated from qmlmin.pro.

#####################################################################
## tst_qmlmin Test:
#####################################################################

add_qt_test(tst_qmlmin
    SOURCES
        tst_qmlmin.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/.\\\"
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmlmin.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmlmin.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qmlmin CONDITION cross_compile AND NOT boot2qt
    DEFINES
        QTEST_CROSS_COMPILED
)
