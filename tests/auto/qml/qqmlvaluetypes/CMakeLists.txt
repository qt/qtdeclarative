# Generated from qqmlvaluetypes.pro.

#####################################################################
## tst_qqmlvaluetypes Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qqmlvaluetypes
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        testtypes.cpp testtypes.h
        tst_qqmlvaluetypes.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlvaluetypes.pro:<TRUE>:
# DISTFILES = "data/customvaluetype.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlvaluetypes CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlvaluetypes CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
