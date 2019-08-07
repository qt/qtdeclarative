# Generated from qqmlpreview.pro.

#####################################################################
## tst_qqmlpreview Test:
#####################################################################

# Collect test data
list(APPEND test_data "data/window.qml")
list(APPEND test_data "data/qtquick2.qml")
list(APPEND test_data "data/window2.qml")
list(APPEND test_data "data/window1.qml")
list(APPEND test_data "data/broken.qml")
list(APPEND test_data "data/zoom.qml")
list(APPEND test_data "data/i18n/qml_fr_FR.qm")

add_qt_test(tst_qqmlpreview
    GUI
    SOURCES
        ../../../../../src/plugins/qmltooling/qmldbg_preview/qqmlpreviewblacklist.cpp ../../../../../src/plugins/qmltooling/qmldbg_preview/qqmlpreviewblacklist.h
        ../../../shared/util.cpp ../../../shared/util.h
        ../shared/debugutil.cpp ../shared/debugutil_p.h
        ../shared/qqmldebugprocess.cpp ../shared/qqmldebugprocess_p.h
        tst_qqmlpreview.cpp
    INCLUDE_DIRECTORIES
        ../../../../../src/plugins/qmltooling/qmldbg_preview
        ../../../shared
        ../shared
    LIBRARIES
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlDebug
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlpreview.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlpreview.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlpreview CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlpreview CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
