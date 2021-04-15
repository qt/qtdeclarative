# Generated from controls.pro.

#####################################################################
## qtquickcontrols2plugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2plugin
    URI "QtQuick.Controls"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuickControls2Plugin
    SKIP_TYPE_REGISTRATION
    IMPORTS
        QtQuick.Controls.impl/auto
    OPTIONAL_IMPORTS
        QtQuick.Controls.Basic/auto
        QtQuick.Controls.Fusion/auto
        QtQuick.Controls.Material/auto
        QtQuick.Controls.Imagine/auto
        QtQuick.Controls.Universal/auto
        QtQuick.Controls.Windows/auto
        QtQuick.Controls.macOS/auto
    SOURCES
        qtquickcontrols2plugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
)

#### Keys ignored in scope 1:.:.:controls.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$PWD/snippets/*.qml" "$$PWD/src/*.qdoc" "$$PWD/src/templates/*.qdoc"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick/Controls"

## Scopes:
#####################################################################

if(QT_FEATURE_quick_designer)
    get_target_property(__aux_qml_files_install_base qtquickcontrols2plugin QT_QML_MODULE_INSTALL_DIR)

    file(GLOB_RECURSE __aux_qml_glob_files
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "designer/images/*.png")
    qt_path_join(__aux_qml_files_install_dir "${__aux_qml_files_install_base}" "designer/images")

    qt_copy_or_install(
        FILES
            ${__aux_qml_glob_files}
        DESTINATION "${__aux_qml_files_install_dir}")

    qt_path_join(__aux_qml_files_install_dir "${__aux_qml_files_install_base}" "designer")

    qt_copy_or_install(
        FILES
            designer/AbstractButtonSection.qml
            designer/BusyIndicatorSpecifics.qml
            designer/ButtonSection.qml
            designer/ButtonSpecifics.qml
            designer/CheckBoxSpecifics.qml
            designer/CheckDelegateSpecifics.qml
            designer/CheckSection.qml
            designer/ComboBoxSpecifics.qml
            designer/ContainerSection.qml
            designer/ControlSection.qml
            designer/ControlSpecifics.qml
            designer/DelayButtonSpecifics.qml
            designer/DialSpecifics.qml
            designer/FrameSpecifics.qml
            designer/GroupBoxSpecifics.qml
            designer/InsetSection.qml
            designer/ItemDelegateSection.qml
            designer/ItemDelegateSpecifics.qml
            designer/LabelSpecifics.qml
            designer/PaddingSection.qml
            designer/PageIndicatorSpecifics.qml
            designer/PageSpecifics.qml
            designer/PaneSection.qml
            designer/PaneSpecifics.qml
            designer/ProgressBarSpecifics.qml
            designer/RadioButtonSpecifics.qml
            designer/RadioDelegateSpecifics.qml
            designer/RangeSliderSpecifics.qml
            designer/RoundButtonSpecifics.qml
            designer/ScrollViewSpecifics.qml
            designer/SliderSpecifics.qml
            designer/SpinBoxSpecifics.qml
            designer/StackViewSpecifics.qml
            designer/SwipeDelegateSpecifics.qml
            designer/SwipeViewSpecifics.qml
            designer/SwitchDelegateSpecifics.qml
            designer/SwitchSpecifics.qml
            designer/TabBarSpecifics.qml
            designer/TabButtonSpecifics.qml
            designer/TextAreaSpecifics.qml
            designer/TextFieldSpecifics.qml
            designer/ToolBarSpecifics.qml
            designer/ToolButtonSpecifics.qml
            designer/ToolSeparatorSpecifics.qml
            designer/TumblerSpecifics.qml
            designer/qtquickcontrols2.metainfo
        DESTINATION "${__aux_qml_files_install_dir}")
endif()
qt_internal_add_docs(qtquickcontrols2plugin
    doc/qtquickcontrols.qdocconf
)

