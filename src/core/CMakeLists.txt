 qt_internal_add_module(QmlCore
    GENERATE_METATYPES
    SOURCES
        qqmlstandardpaths_p.h
        qqmlstandardpaths.cpp
        qqmlcoreglobal_p.h
    DEFINES
        QT_BUILD_QML_CORE_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

set_target_properties(QmlCore PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtCore
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtCore"
)

qt6_qml_type_registration(QmlCore)

qt_internal_add_docs(QmlCore
    doc/qtqmlcore.qdocconf
)
