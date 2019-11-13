# Generated from qquickanimationcontroller.pro.

#####################################################################
## tst_qquickanimationcontroller Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickanimationcontroller
    GUI
    QMLTEST
    SOURCES
        tst_qquickanimationcontroller.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickanimationcontroller.pro:<TRUE>:
# TEMPLATE = "app"
