# Generated from qqmlinfo.pro.

#####################################################################
## tst_qqmlinfo Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlinfo
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlinfo.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlinfo.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlinfo.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlinfo CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlinfo CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
