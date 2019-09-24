# Generated from qquickdrawer.pro.

#####################################################################
## tst_qquickdrawer Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickdrawer
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickdrawer.cpp
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
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickdrawer.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"

## Scopes:
#####################################################################

extend_target(tst_qquickdrawer CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickdrawer CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
