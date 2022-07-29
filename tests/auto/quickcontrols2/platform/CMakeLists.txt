# Generated from platform.pro.

#####################################################################
## tst_platform Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_platform
    GUI
    QMLTEST
    SOURCES
        tst_platform.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:platform.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"
