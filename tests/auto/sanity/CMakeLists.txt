# Generated from sanity.pro.

#####################################################################
## tst_sanity Test:
#####################################################################

add_qt_test(tst_sanity
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_sanity.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickTemplates2
)

#### Keys ignored in scope 1:.:.:sanity.pro:<TRUE>:
# OTHER_FILES = "$$PWD/BLACKLIST"
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(tst_sanity CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_sanity CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
