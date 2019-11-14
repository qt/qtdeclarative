# Generated from qquickaccessible.pro.

#####################################################################
## tst_qquickaccessible Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickaccessible
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickaccessible.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickaccessible.pro:<TRUE>:
# OTHER_FILES = "data/checkbuttons.qml" "data/hittest.qml" "data/pushbutton.qml" "data/statictext.qml" "data/ignored.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickaccessible CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickaccessible CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
