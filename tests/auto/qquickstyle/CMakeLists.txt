# Generated from qquickstyle.pro.

#####################################################################
## tst_qquickstyle Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickstyle
    SOURCES
        ../shared/qtest_quickcontrols.h
        ../shared/util.cpp ../shared/util.h
        ../shared/visualtestutil.cpp ../shared/visualtestutil.h
        tst_qquickstyle.cpp
    DEFINES
        QQC2_IMPORT_PATH=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../../src/imports\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickControls2Private
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::TestPrivate
    TESTDATA ${test_data}
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "qrcStyles1/QrcStyle1/*.qml")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

file(GLOB resource_glob_1 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "qrcStyles2/QrcStyle2/*.qml")
foreach(file IN LISTS resource_glob_1)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

file(GLOB resource_glob_2 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "qrcStyles3/QrcStyle3/*.qml")
foreach(file IN LISTS resource_glob_2)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

file(GLOB resource_glob_3 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "qrcStyles4/QrcStyle4/*.qml")
foreach(file IN LISTS resource_glob_3)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_qrcStyles1_resource_files
    ${resource_glob_0}
)

qt_add_resource(tst_qquickstyle "qmake_qrcStyles1"
    PREFIX
        "/"
    FILES
        ${qmake_qrcStyles1_resource_files}
)
set(qmake_qrcStyles2_resource_files
    ${resource_glob_1}
)

qt_add_resource(tst_qquickstyle "qmake_qrcStyles2"
    PREFIX
        "/"
    FILES
        ${qmake_qrcStyles2_resource_files}
)
set(qmake_qrcStyles3_resource_files
    ${resource_glob_2}
)

qt_add_resource(tst_qquickstyle "qmake_qrcStyles3"
    PREFIX
        "/"
    FILES
        ${qmake_qrcStyles3_resource_files}
)
set(qmake_qrcStyles4_resource_files
    ${resource_glob_3}
)

qt_add_resource(tst_qquickstyle "qmake_qrcStyles4"
    PREFIX
        "/"
    FILES
        ${qmake_qrcStyles4_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(tst_qquickstyle CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickstyle CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
