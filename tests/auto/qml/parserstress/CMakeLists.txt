# Generated from parserstress.pro.

#####################################################################
## tst_parserstress Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "tests/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_parserstress
    GUI
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

#### Keys ignored in scope 1:.:.:parserstress.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:parserstress.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
