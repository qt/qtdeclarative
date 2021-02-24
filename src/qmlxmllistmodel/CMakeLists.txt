qt_internal_add_module(QmlXmlListModel
    GENERATE_METATYPES
    SOURCES
        qqmlxmllistmodel_p.h qqmlxmllistmodel.cpp
        qtqmlxmllistmodelglobal_p.h
    DEFINES
        QT_BUILD_QMLXMLLISTMODEL_LIB
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

qt_internal_add_docs(QmlXmlListModel
    doc/qtqmlxmllistmodel.qdocconf
)

set_target_properties(QmlXmlListModel PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQml.XmlListModel
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQml/XmlListModel"
)

qt6_qml_type_registration(QmlXmlListModel)
