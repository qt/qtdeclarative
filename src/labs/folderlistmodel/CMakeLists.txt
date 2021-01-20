qt_internal_add_module(LabsFolderListModel
  GENERATE_METATYPES
  SOURCES
        fileinfothread.cpp fileinfothread_p.h
        fileproperty_p.h
        qquickfolderlistmodel.cpp qquickfolderlistmodel.h
        qquickfolderlistmodelglobal_p.h
  DEFINES
        QT_BUILD_LABSFOLDERMODEL_LIB
  PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

set_target_properties(LabsFolderListModel PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.folderlistmodel
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/folderlistmodel"
)


qt6_qml_type_registration(LabsFolderListModel)
