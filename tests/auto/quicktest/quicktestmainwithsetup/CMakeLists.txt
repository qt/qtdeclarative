# Generated from quicktestmainwithsetup.pro.

#####################################################################
## tst_quicktestmainwithsetup Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_quicktestmainwithsetup
    GUI
    QMLTEST
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_quicktestmainwithsetup.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:quicktestmainwithsetup.pro:<TRUE>:
# CONFIG = "qmltestcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:quicktestmainwithsetup.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_quicktestmainwithsetup CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_quicktestmainwithsetup CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
