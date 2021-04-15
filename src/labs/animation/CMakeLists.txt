qt_internal_add_module(LabsAnimation
  GENERATE_METATYPES
  SOURCES
      qquickboundaryrule.cpp qquickboundaryrule_p.h
      qqmlanimationglobal_p.h
  DEFINES
      QT_BUILD_LABSANIMATION_LIB
  PUBLIC_LIBRARIES
      Qt::QmlPrivate
      Qt::QuickPrivate
)

set_target_properties(LabsAnimation PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI Qt.labs.animation
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/Qt/labs/animation"
)

qt6_qml_type_registration(LabsAnimation)
