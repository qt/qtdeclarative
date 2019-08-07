# Generated from qquickdraghandler.pro.

#####################################################################
## tst_qquickdraghandler Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickdraghandler
    GUI
    SOURCES
        ../../../shared/util.cpp ../../../shared/util.h
        ../../shared/geometrytestutil.cpp ../../shared/geometrytestutil.h
        ../../shared/viewtestutil.cpp ../../shared/viewtestutil.h
        ../../shared/visualtestutil.cpp ../../shared/visualtestutil.h
        tst_qquickdraghandler.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    INCLUDE_DIRECTORIES
        ../../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickdraghandler.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/DragAnywhereSlider.qml" "data/FlashAnimation.qml" "data/Slider.qml" "data/draggables.qml" "data/grabberstate.qml" "data/multipleSliders.qml" "data/reparenting.qml" "data/snapMode.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquickdraghandler.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquickdraghandler CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickdraghandler CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
