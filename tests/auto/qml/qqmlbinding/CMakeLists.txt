# Generated from qqmlbinding.pro.

#####################################################################
## tst_qqmlbinding Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlbinding
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlbinding.cpp
        WithBindableProperties.h
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    TESTDATA ${test_data}
)

set_target_properties(tst_qqmlbinding PROPERTIES
    QT_QML_MODULE_URI "test"
    QT_QML_MODULE_VERSION 1.0
)

qt6_qml_type_registration(tst_qqmlbinding)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlbinding CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlbinding CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
