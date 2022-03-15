# Generated from material.pro.

#####################################################################
## tst_material Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_material
    GUI
    QMLTEST
    SOURCES
        tst_material.cpp
    DEFINES
        TST_CONTROLS_DATA=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../data\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QuickControls2
    TESTDATA ${test_data}
)

# Make the QML files available to Creator's locator.
target_sources(tst_material
    PRIVATE
        ${test_data}
)

set_source_files_properties(${test_data}
    PROPERTIES
        HEADER_FILE_ONLY ON
)

#### Keys ignored in scope 1:.:.:material.pro:<TRUE>:
# OTHER_FILES = "$$PWD/../data/*.qml"
# TEMPLATE = "app"
