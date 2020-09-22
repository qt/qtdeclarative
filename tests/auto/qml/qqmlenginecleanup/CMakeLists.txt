# Generated from qqmlenginecleanup.pro.

#####################################################################
## tst_qqmlenginecleanup Test:
#####################################################################

qt_internal_add_test(tst_qqmlenginecleanup
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlenginecleanup.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlenginecleanup CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlenginecleanup CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
add_subdirectory(CustomModuleImport)
