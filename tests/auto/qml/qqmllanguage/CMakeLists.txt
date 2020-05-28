# Generated from qqmllanguage.pro.

#####################################################################
## tst_qqmllanguage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qqmllanguage
    SOURCES
        ../../shared/testhttpserver.cpp ../../shared/testhttpserver.h
        ../../shared/util.cpp ../../shared/util.h
        testtypes.cpp testtypes.h
        tst_qqmllanguage.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmllanguage.pro:<TRUE>:
# OTHER_FILES = "data/readonlyObjectProperty.qml"
# QML_IMPORT_NAME = "StaticTest"
# QML_IMPORT_VERSION = "1.0"

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set_source_files_properties("data/I18nType30.qml"
        PROPERTIES QT_RESOURCE_ALIAS "data/I18nTypeÁâãäå.qml"
    )
    set(qqmllanguage_resource_files
        "data/I18nType30.qml"
    )

    qt_add_resource(tst_qqmllanguage "qqmllanguage"
        PREFIX
            "/"
        FILES
            ${qqmllanguage_resource_files}
    )
endif()

qt_extend_target(tst_qqmllanguage CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmllanguage CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

set_target_properties(tst_qqmllanguage PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI StaticTest
)

qt6_qml_type_registration(tst_qqmllanguage)
