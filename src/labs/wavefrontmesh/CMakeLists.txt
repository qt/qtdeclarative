qt_internal_add_module(LabsWavefrontMesh
  GENERATE_METATYPES
  SOURCES
      qwavefrontmesh.cpp qwavefrontmesh.h
      qqmlwavefrontmeshglobal_p.h
  DEFINES
        QT_BUILD_LABSWAVEFRONTMESH_LIB
  PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
)


set_target_properties(LabsWavefrontMesh PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.wavefrontmesh
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/wavefrontmesh"
)

qt6_qml_type_registration(LabsWavefrontMesh)
