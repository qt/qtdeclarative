# Generated from creationtime.pro.

#####################################################################
## tst_creationtime Test:
#####################################################################

qt_add_test(tst_creationtime
    SOURCES
        ../../auto/shared/qtest_quickcontrols.h
        ../../auto/shared/util.cpp ../../auto/shared/util.h
        ../../auto/shared/visualtestutil.cpp ../../auto/shared/visualtestutil.h
        tst_creationtime.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
)

#### Keys ignored in scope 1:.:.:creationtime.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_extend_target(tst_creationtime CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_creationtime CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
