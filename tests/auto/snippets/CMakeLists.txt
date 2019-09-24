# Generated from snippets.pro.

#####################################################################
## tst_snippets Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_snippets
    SOURCES
        tst_snippets.cpp
    DEFINES
        QQC2_SNIPPETS_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports/controls/doc/snippets\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:snippets.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"

## Scopes:
#####################################################################
