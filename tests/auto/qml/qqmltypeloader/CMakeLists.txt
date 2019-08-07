# Generated from qqmltypeloader.pro.

#####################################################################
## tst_qqmltypeloader Test:
#####################################################################

add_qt_test(tst_qqmltypeloader
    GUI
    SOURCES
        ../../shared/testhttpserver.cpp ../../shared/testhttpserver.h
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmltypeloader.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 2:.:.:tst_qqmltypeloader.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:tst_qqmltypeloader.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmltypeloader CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmltypeloader CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
add_subdirectory(SlowImport)
