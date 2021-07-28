# Generated from imagine.pro.

#####################################################################
## tst_imagine Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_imagine
    GUI
    QMLTEST
    SOURCES
        tst_imagine.cpp
    DEFINES
        TST_CONTROLS_DATA=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../data\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QuickControls2
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:imagine.pro:<TRUE>:
# OTHER_FILES = "$$PWD/../data/*.qml"
# TEMPLATE = "app"
