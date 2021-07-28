# Generated from snippets.pro.

#####################################################################
## tst_snippets Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_snippets
    SOURCES
        tst_snippets.cpp
    DEFINES
        QQC2_SNIPPETS_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols2/doc/snippets\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickControls2Private
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(tst_snippets "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:snippets.pro:<TRUE>:
# OTHER_FILES = "$$PWD/data/*.qml"
# TEMPLATE = "app"

## Scopes:
#####################################################################
