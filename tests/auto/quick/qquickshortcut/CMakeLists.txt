# Generated from qquickshortcut.pro.

#####################################################################
## tst_qquickshortcut Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickshortcut
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
