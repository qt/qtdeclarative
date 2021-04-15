qt_internal_add_module(LabsSettings
    GENERATE_METATYPES
    SOURCES
         qqmlsettings.cpp qqmlsettings_p.h
         qqmlsettingsglobal_p.h
    DEFINES
         QT_BUILD_LABSSETTINGS_LIB
    PUBLIC_LIBRARIES
         Qt::Core
         Qt::Qml
)

set_target_properties(LabsSettings PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.settings
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/settings"
)

qt6_qml_type_registration(LabsSettings)
