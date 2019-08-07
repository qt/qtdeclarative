# Generated from polish-qml.pro.

#####################################################################
## tst_polish-qml Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_polish-qml
    GUI
    QMLTEST
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_polish-qml.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:polish-qml.pro:<TRUE>:
# CONFIG = "qmltestcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:polish-qml.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_polish-qml CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_polish-qml CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
