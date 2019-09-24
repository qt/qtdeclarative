# Generated from default.pro.

#####################################################################
## tst_default Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tst_*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_default
    GUI
    QMLTEST
    SOURCES
        tst_default.cpp
    DEFINES
        TST_CONTROLS_DATA=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../tests/auto/controls/data\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:default.pro:<TRUE>:
# OTHER_FILES = "$$PWD/../data/*.qml"
# TEMPLATE = "app"
