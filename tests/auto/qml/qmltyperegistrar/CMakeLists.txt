# Generated from qmltyperegistrar.pro.

#####################################################################
## tst_qmltyperegistrar Test:
#####################################################################

qt_manual_moc(moc_files OUTPUT_MOC_JSON_FILES json_list noextheader
              INCLUDE_DIRECTORY_TARGETS Qt::Qml)

# Dummy target to pass --private-includes to qmltyperegistrar for tst_qmltyperegistrar.
# We want to test that it expects files named foo_p.h appearing in foreign metatypes
# to be in a private/ subdirectory. In particular, we have a foreign_p.h to exercise this.
# TODO: There should be more fine grained control over this.
qt_add_library(tst_qmltyperegistrarPrivate STATIC
    dummy.cpp dummy_p.h
)

target_link_libraries(tst_qmltyperegistrarPrivate PRIVATE Qt::Core)

qt_internal_add_test(tst_qmltyperegistrar
    SOURCES
        hppheader.hpp
        # noextheader special case
        tst_qmltyperegistrar.cpp tst_qmltyperegistrar.h
        foo.cpp foo.h
        ${moc_files}
    INCLUDE_DIRECTORIES
        foreign
    PUBLIC_LIBRARIES
        # Remove: Lforeign
        Qt::Qml
        foreign
        tst_qmltyperegistrarPrivate
)

#### Keys ignored in scope 2:.:.:tst_qmltyperegistrar.pro:<TRUE>:
# QMLTYPES_FILENAME = "tst_qmltyperegistrar.qmltypes"
# QML_FOREIGN_METATYPES = "foreign/foreign_metatypes.json"
# QML_IMPORT_NAME = "QmlTypeRegistrarTest"
# QML_IMPORT_VERSION = "1.0"
# QML_PAST_MAJOR_VERSIONS = "0"
# TEMPLATE = "app"

## Scopes:
#####################################################################

# Simulate conditions that qt6_add_qml_module() would normally set up for us
set_target_properties(tst_qmltyperegistrar PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_PAST_MAJOR_VERSIONS 0
    QT_QML_MODULE_URI QmlTypeRegistrarTest
    QT_QML_MODULE_TYPEINFO tst_qmltyperegistrar.qmltypes
    QT_QML_MODULE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# qt6_add_qml_module() doesn't allow us to pass through MANUAL_MOC_JSON_FILES
# yet, so we have to call it directly to test that code path for now.
_qt_internal_qml_type_registration(tst_qmltyperegistrar MANUAL_MOC_JSON_FILES ${json_list})
add_subdirectory(foreign)
