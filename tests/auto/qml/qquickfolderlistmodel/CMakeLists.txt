# Generated from qquickfolderlistmodel.pro.

#####################################################################
## tst_qquickfolderlistmodel Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickfolderlistmodel
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickfolderlistmodel.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

# special case begin
set_source_files_properties(data/txtdir/hello.txt PROPERTIES QT_RESOURCE_ALIAS "txtdir/hello.txt")
# special case end
# Resources:
set(introspect_resource_files
    "txtdir/hello.txt"
)

qt_add_resource(tst_qquickfolderlistmodel "introspect"
    PREFIX
        "/myprefix/subdir"
    BASE
        "data"
    FILES
        ${introspect_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(tst_qquickfolderlistmodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickfolderlistmodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
