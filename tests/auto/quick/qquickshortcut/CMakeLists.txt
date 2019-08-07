# Generated from qquickshortcut.pro.

#####################################################################
## tst_qquickshortcut Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickshortcut
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickshortcut.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickshortcut.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qquickshortcut CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::QuickWidgets
)

extend_target(tst_qquickshortcut CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickshortcut CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
