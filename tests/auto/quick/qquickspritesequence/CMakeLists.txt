# Generated from qquickspritesequence.pro.

#####################################################################
## tst_qquickspritesequence Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickspritesequence
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickspritesequence.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickspritesequence.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquickspritesequence.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquickspritesequence CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickspritesequence CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
