# Generated from qqmlvaluetypes.pro.

#####################################################################
## tst_qqmlvaluetypes Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlvaluetypes
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        testtypes.cpp testtypes.h
        tst_qqmlvaluetypes.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlvaluetypes.pro:<TRUE>:
# DISTFILES = "data/customvaluetype.qml"

## Scopes:
#####################################################################

extend_target(tst_qqmlvaluetypes CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlvaluetypes CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
