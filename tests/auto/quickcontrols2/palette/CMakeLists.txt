# Generated from palette.pro.

#####################################################################
## tst_palette Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_palette
    SOURCES
        tst_palette.cpp
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
set(qmake_immediate_resource_files
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(tst_palette "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:palette.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_palette CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_palette CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
