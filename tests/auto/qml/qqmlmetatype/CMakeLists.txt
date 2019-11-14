# Generated from qqmlmetatype.pro.

#####################################################################
## tst_qqmlmetatype Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qqmlmetatype
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
set(qmake_qmlfiles_resource_files
    "data/CompositeType.qml"
)

qt_add_resource(tst_qqmlmetatype "qmake_qmlfiles"
    PREFIX
        "/tstqqmlmetatype"
    FILES
        ${qmake_qmlfiles_resource_files}
)
set(qmake_qmldirresource_resource_files
    "data/Components/App.qml"
    "data/Components/qmldir"
    "data/enumsInRecursiveImport.qml"
)

qt_add_resource(tst_qqmlmetatype "qmake_qmldirresource"
    PREFIX
        "/"
    FILES
        ${qmake_qmldirresource_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(tst_qqmlmetatype CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlmetatype CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
