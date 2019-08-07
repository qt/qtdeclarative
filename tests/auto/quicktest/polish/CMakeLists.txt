# Generated from polish.pro.

#####################################################################
## tst_polish Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_polish
    GUI
    QMLTEST
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_polish.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:polish.pro:<TRUE>:
# CONFIG = "qmltestcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:polish.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_polish CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_polish CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
