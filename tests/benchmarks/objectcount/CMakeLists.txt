# Generated from objectcount.pro.

#####################################################################
## tst_objectcount Test:
#####################################################################

qt_add_test(tst_objectcount
    SOURCES
        ../../auto/shared/qtest_quickcontrols.h
        ../../auto/shared/util.cpp ../../auto/shared/util.h
        ../../auto/shared/visualtestutil.cpp ../../auto/shared/visualtestutil.h
        tst_objectcount.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
)

#### Keys ignored in scope 1:.:.:objectcount.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_extend_target(tst_objectcount CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_objectcount CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
