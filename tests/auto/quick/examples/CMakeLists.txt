# Generated from examples.pro.

#####################################################################
## tst_examples Test:
#####################################################################

add_qt_test(tst_examples
    SOURCES
        tst_examples.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
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

#### Keys ignored in scope 1:.:.:examples.pro:<TRUE>:
# testcase.timeout = "400"

## Scopes:
#####################################################################
