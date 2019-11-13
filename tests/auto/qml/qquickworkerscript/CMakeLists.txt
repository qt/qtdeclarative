# Generated from qquickworkerscript.pro.

#####################################################################
## tst_qquickworkerscript Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickworkerscript
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickworkerscript.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QmlWorkerScriptPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlWorkerScript
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qquickworkerscript CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickworkerscript CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
