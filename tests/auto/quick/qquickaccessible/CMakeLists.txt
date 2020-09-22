# Generated from qquickaccessible.pro.

#####################################################################
## tst_qquickaccessible Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickaccessible
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickaccessible.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
        Qt::QuickPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickaccessible.pro:<TRUE>:
# OTHER_FILES = "data/checkbuttons.qml" "data/hittest.qml" "data/pushbutton.qml" "data/statictext.qml" "data/ignored.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickaccessible CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickaccessible CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
