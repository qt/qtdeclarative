qt_internal_add_module(LabsQmlModels
  GENERATE_METATYPES
  SOURCES
       qqmlmodelsglobal_p.h
  DEFINES
       QT_BUILD_LABSQMLMODELS_LIB
  PUBLIC_LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
  )

qt_internal_extend_target(LabsQmlModels CONDITION QT_FEATURE_qml_table_model
    SOURCES
        qqmltablemodel.cpp qqmltablemodel_p.h
        qqmltablemodelcolumn.cpp qqmltablemodelcolumn_p.h
)

qt_internal_extend_target(LabsQmlModels CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        qqmldelegatecomponent.cpp qqmldelegatecomponent_p.h
)

set_target_properties(LabsQmlModels PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.qmlmodels
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/qmlmodels"

)

qt6_qml_type_registration(LabsQmlModels)
