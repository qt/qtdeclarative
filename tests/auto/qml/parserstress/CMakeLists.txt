# Generated from parserstress.pro.

#####################################################################
## tst_parserstress Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    tests/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_parserstress
    SOURCES
        tst_parserstress.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################
