# Generated from calendar.pro.

#####################################################################
## tst_calendar Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_calendar
    GUI
    QMLTEST
    SOURCES
        tst_calendar.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:calendar.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"
