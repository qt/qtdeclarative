# Generated from quicktestmain.pro.

#####################################################################
## quicktestmain Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qml")
list(APPEND test_data ${test_data_glob})

add_qt_test(quicktestmain
    GUI
    QMLTEST
    SOURCES
        quicktestmain.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:quicktestmain.pro:<TRUE>:
# CONFIG = "qmltestcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:quicktestmain.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
