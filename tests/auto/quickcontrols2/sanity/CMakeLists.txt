# Generated from sanity.pro.

#####################################################################
## tst_sanity Test:
#####################################################################

qt_internal_add_test(tst_sanity
    SOURCES
        tst_sanity.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols2\\\"
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
        Qt::QmlCompilerPrivate
)

#### Keys ignored in scope 1:.:.:sanity.pro:<TRUE>:
# OTHER_FILES = "$$PWD/BLACKLIST"
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_sanity CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_sanity CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

qt_internal_extend_target(tst_sanity CONDITION WEBOS
    DEFINES
        QQC2_IMPORT_PATH=\\\"/usr/lib/qml/QtQuick/Controls\\\"
)
