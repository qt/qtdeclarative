# Generated from qquickuniversalstyleconf.pro.

#####################################################################
## tst_qquickuniversalstyleconf Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickuniversalstyleconf
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickuniversalstyleconf.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qquickuniversalstyleconf_resource_files
    "qtquickcontrols2.conf"
)

qt_add_resource(tst_qquickuniversalstyleconf "qquickuniversalstyleconf"
    PREFIX
        "/"
    FILES
        ${qquickuniversalstyleconf_resource_files}
)


#### Keys ignored in scope 1:.:.:qquickuniversalstyleconf.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickuniversalstyleconf CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickuniversalstyleconf CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
