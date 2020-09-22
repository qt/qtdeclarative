# Generated from qmltyperegistrar.pro.

#####################################################################
## tst_qmltyperegistrar Test:
#####################################################################

qt_manual_moc(moc_files OUTPUT_MOC_JSON_FILES json_list noextheader) # special case
qt_internal_add_test(tst_qmltyperegistrar
    SOURCES
        hppheader.hpp
        # noextheader special case
        tst_qmltyperegistrar.cpp tst_qmltyperegistrar.h
    INCLUDE_DIRECTORIES
        foreign
    PUBLIC_LIBRARIES
        # Remove: Lforeign
        Qt::Qml
        foreign
)
target_compile_definitions(tst_qmltyperegistrar PRIVATE BUILD_WITH_CMAKE) # special case

#### Keys ignored in scope 2:.:.:tst_qmltyperegistrar.pro:<TRUE>:
# QMLTYPES_FILENAME = "tst_qmltyperegistrar.qmltypes"
# QML_FOREIGN_METATYPES = "foreign/foreign_metatypes.json"
# QML_IMPORT_NAME = "QmlTypeRegistrarTest"
# QML_IMPORT_VERSION = "1.0"
# TEMPLATE = "app"

## Scopes:
#####################################################################

set_target_properties(tst_qmltyperegistrar PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI QmlTypeRegistrarTest
    QT_QMLTYPES_FILENAME tst_qmltyperegistrar.qmltypes
)

qt6_qml_type_registration(tst_qmltyperegistrar ${json_list}) # special case
add_subdirectory(foreign)
