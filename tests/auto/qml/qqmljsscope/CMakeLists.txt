qt_internal_add_test(tst_qqmljsscope
    SOURCES
        tst_qqmljsscope.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA
        data/orderedBindings.qml
)

add_dependencies(tst_qqmljsscope Qt::Quick) # we need QtQuick QML module

qt_internal_extend_target(tst_qqmljsscope CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmljsscope CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
