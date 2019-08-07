# Generated from drawingmodes.pro.

#####################################################################
## tst_drawingmodes Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_drawingmodes
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_drawingmodes.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:drawingmodes.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test"
# OTHER_FILES = "data/DrawingModes.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:drawingmodes.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_drawingmodes CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_drawingmodes CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
