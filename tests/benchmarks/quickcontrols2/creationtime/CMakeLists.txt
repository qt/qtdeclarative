# Generated from creationtime.pro.

#####################################################################
## tst_creationtime Test:
#####################################################################

qt_internal_add_test(tst_creationtime
    SOURCES
        ../../../auto/quickcontrols2/shared/qtest_quickcontrols.h
        ../../../auto/quickcontrols2/shared/util.cpp ../../../auto/quickcontrols2/shared/util.h
        ../../../auto/quickcontrols2/shared/visualtestutil.cpp ../../../auto/quickcontrols2/shared/visualtestutil.h
        tst_creationtime.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::TestPrivate
)

#### Keys ignored in scope 1:.:.:creationtime.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_creationtime CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_creationtime CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
