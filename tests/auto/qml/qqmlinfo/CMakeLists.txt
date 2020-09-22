# Generated from qqmlinfo.pro.

#####################################################################
## tst_qqmlinfo Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlinfo
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        attached.cpp attached.h
        tst_qqmlinfo.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlinfo.pro:<TRUE>:
# QML_IMPORT_MAJOR_VERSION = "1"
# QML_IMPORT_NAME = "org.qtproject.Test"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlinfo CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlinfo CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)

set_target_properties(tst_qqmlinfo PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI org.qtproject.Test
)

qt6_qml_type_registration(tst_qqmlinfo)
