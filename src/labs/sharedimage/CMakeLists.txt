qt_internal_add_module(LabsSharedImage
  GENERATE_METATYPES
  SOURCES
        qsharedimageloader.cpp qsharedimageloader_p.h
        qsharedimageprovider.cpp qsharedimageprovider_p.h
        qtlabssharedimageglobal_p.h
  DEFINES
        QT_BUILD_LABSSHAREDIMAGE_LIB
  PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
)


set_target_properties(LabsSharedImage PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.sharedimage
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/sharedimage"
)

qt6_qml_type_registration(LabsSharedImage)
