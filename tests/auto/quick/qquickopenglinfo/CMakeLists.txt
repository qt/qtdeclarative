# Generated from qquickopenglinfo.pro.

#####################################################################
## tst_qquickopenglinfo Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickopenglinfo
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickopenglinfo.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickopenglinfo.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/basic.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquickopenglinfo.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquickopenglinfo CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickopenglinfo CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
