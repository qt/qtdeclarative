# Generated from qquickmaterialstyleconf.pro.

#####################################################################
## tst_qquickmaterialstyleconf Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickmaterialstyleconf
    SOURCES
        tst_qquickmaterialstyleconf.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols2\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qquickmaterialstyleconf_resource_files
    "qtquickcontrols2.conf"
    "variant-dense.conf"
    "variant-normal.conf"
)

qt_internal_add_resource(tst_qquickmaterialstyleconf "qquickmaterialstyleconf"
    PREFIX
        "/"
    FILES
        ${qquickmaterialstyleconf_resource_files}
)


#### Keys ignored in scope 1:.:.:qquickmaterialstyleconf.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickmaterialstyleconf CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickmaterialstyleconf CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
