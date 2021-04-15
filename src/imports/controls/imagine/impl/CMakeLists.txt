# Generated from imagine-impl.pro.

#####################################################################
## qtquickcontrols2imaginestyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2imaginestyleimplplugin
    URI "QtQuick.Controls.Imagine.impl"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2ImagineStyleImplPlugin
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        qquickimageselector.cpp qquickimageselector_p.h
        qquickninepatchimage.cpp qquickninepatchimage_p.h
        qtquickcontrols2imaginestyleimplplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qmake_qtquickcontrols2imaginestyleimpl_resource_files
    "shaders/+glslcore/OpacityMask.frag"
    "shaders/+qsb/OpacityMask.frag"
    "shaders/OpacityMask.frag"
)

qt_internal_add_resource(qtquickcontrols2imaginestyleimplplugin "qmake_qtquickcontrols2imaginestyleimpl"
    PREFIX
        "qt-project.org/imports/QtQuick/Controls/Imagine/impl"
    FILES
        ${qmake_qtquickcontrols2imaginestyleimpl_resource_files}
)


#### Keys ignored in scope 1:.:.:imagine-impl.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_FILES = "$$PWD/OpacityMask.qml"
# QML_IMPORT_NAME = "QtQuick.Controls.Imagine.impl"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# QT_FOR_CONFIG = "quickcontrols2-private"
# TARGETPATH = "QtQuick/Controls/Imagine/impl"
# _REQUIREMENTS = "qtConfig(quickcontrols2-imagine)"

set(qml_files
    "OpacityMask.qml"
)
set_source_files_properties(OpacityMask.qml PROPERTIES
    QT_QML_SOURCE_VERSION "6.0"
)

qt6_target_qml_files(qtquickcontrols2imaginestyleimplplugin
    FILES
        ${qml_files}
)
