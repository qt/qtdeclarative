# Generated from qqmlenginecleanup.pro.

#####################################################################
## tst_qqmlenginecleanup Test:
#####################################################################

add_qt_test(tst_qqmlenginecleanup
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlenginecleanup.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
)

#### Keys ignored in scope 2:.:.:tst_qqmlenginecleanup.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:tst_qqmlenginecleanup.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlenginecleanup CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlenginecleanup CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
add_subdirectory(CustomModuleImport)
