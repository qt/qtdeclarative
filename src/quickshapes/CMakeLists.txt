# Generated from quickshapes.pro.

#####################################################################
## QuickShapes Module:
#####################################################################

qt_add_module(QuickShapes
    INTERNAL_MODULE
    GENERATE_METATYPES
    SOURCES
        qquickshape.cpp qquickshape_p.h
        qquickshape_p_p.h
        qquickshapegenericrenderer.cpp qquickshapegenericrenderer_p.h
        qquickshapesglobal.h qquickshapesglobal_p.h
        qquickshapesoftwarerenderer.cpp qquickshapesoftwarerenderer_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickPrivate
)

# Resources:
set(qtquickshapes_resource_files
    "shaders_ng/conicalgradient.frag.qsb"
    "shaders_ng/conicalgradient.vert.qsb"
    "shaders_ng/lineargradient.frag.qsb"
    "shaders_ng/lineargradient.vert.qsb"
    "shaders_ng/radialgradient.frag.qsb"
    "shaders_ng/radialgradient.vert.qsb"
)

qt_add_resource(QuickShapes "qtquickshapes"
    PREFIX
        "/qt-project.org/shapes"
    FILES
        ${qtquickshapes_resource_files}
)


#### Keys ignored in scope 1:.:.:quickshapes.pro:<TRUE>:
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick/Shapes"
# QML_IMPORT_NAME = "QtQuick.Shapes"
# QML_IMPORT_VERSION = "$$QT_VERSION"

set_target_properties(QuickShapes PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Shapes
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Shapes"
)

qt6_qml_type_registration(QuickShapes)
