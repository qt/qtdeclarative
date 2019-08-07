# Generated from qqmlmetatype.pro.

#####################################################################
## tst_qqmlmetatype Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlmetatype
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlmetatype.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
add_qt_resource(tst_qqmlmetatype "qmake_qmlfiles"
    PREFIX
        "/tstqqmlmetatype"
    FILES
        data/CompositeType.qml
)
add_qt_resource(tst_qqmlmetatype "qmake_qmldirresource"
    PREFIX
        "/"
    FILES
        data/Components/App.qml
        data/Components/qmldir
        data/enumsInRecursiveImport.qml
)


#### Keys ignored in scope 1:.:.:qqmlmetatype.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlmetatype.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlmetatype CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlmetatype CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
