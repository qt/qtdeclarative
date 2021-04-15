# Generated from nativestyle.pro.

#####################################################################
## qtquickcontrols2nativestyleplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquickcontrols2nativestyleplugin
    URI "QtQuick.NativeStyle"
    VERSION "${PROJECT_VERSION}"
    CLASSNAME QtQuickControls2NativeStylePlugin
    DEPENDENCIES
        QtQuick.Controls/auto
        QtQuick.Layouts/auto
    PAST_MAJOR_VERSIONS 2
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        items/qquickstyleitem.cpp items/qquickstyleitem.h
        items/qquickstyleitembutton.cpp items/qquickstyleitembutton.h
        items/qquickstyleitemcheckbox.cpp items/qquickstyleitemcheckbox.h
        items/qquickstyleitemcombobox.cpp items/qquickstyleitemcombobox.h
        items/qquickstyleitemdial.cpp items/qquickstyleitemdial.h
        items/qquickstyleitemframe.cpp items/qquickstyleitemframe.h
        items/qquickstyleitemgroupbox.cpp items/qquickstyleitemgroupbox.h
        items/qquickstyleitemprogressbar.cpp items/qquickstyleitemprogressbar.h
        items/qquickstyleitemradiobutton.cpp items/qquickstyleitemradiobutton.h
        items/qquickstyleitemscrollbar.cpp items/qquickstyleitemscrollbar.h
        items/qquickstyleitemslider.cpp items/qquickstyleitemslider.h
        items/qquickstyleitemspinbox.cpp items/qquickstyleitemspinbox.h
        items/qquickstyleitemtextfield.cpp items/qquickstyleitemtextfield.h
        qstyle/qquickcommonstyle.cpp qstyle/qquickcommonstyle.h qstyle/qquickcommonstyle_p.h
        qstyle/qquickcommonstylepixmaps_p.h
        qstyle/qquickdrawutil.cpp qstyle/qquickdrawutil.h
        qstyle/qquicknativestyle.cpp qstyle/qquicknativestyle.h
        qstyle/qquickstyle.cpp qstyle/qquickstyle.h qstyle/qquickstyle_p.h
        qstyle/qquickstylehelper.cpp qstyle/qquickstylehelper_p.h
        qstyle/qquickstyleoption.cpp qstyle/qquickstyleoption.h
        qtquickcontrols2nativestyleplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        items
        qstyle
        util
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickTemplates2
)

#### Keys ignored in scope 1:.:.:nativestyle.pro:<TRUE>:
# OTHER_FILES = "qmldir" "$$QML_FILES"
# QML_IMPORT_MAJOR_VERSION = "6"
# QML_IMPORT_NAME = "QtQuick.NativeStyle"
# QML_PAST_MAJOR_VERSIONS = "2"
# TARGETPATH = "QtQuick/NativeStyle"
# _REQUIREMENTS = "qtConfig(quickcontrols2-macos)|qtConfig(quickcontrols2-windows)"

## Scopes:
#####################################################################

qt_internal_extend_target(qtquickcontrols2nativestyleplugin CONDITION MACOS
    SOURCES
        items/qquickstyleitemscrollviewcorner.cpp items/qquickstyleitemscrollviewcorner.h
        qstyle/mac/qquickmacstyle_mac.mm qstyle/mac/qquickmacstyle_mac_p.h
        qstyle/mac/qquickmacstyle_mac_p_p.h
        util/qquickmacfocusframe.h util/qquickmacfocusframe.mm
    INCLUDE_DIRECTORIES
        qstyle/mac
    LIBRARIES
        ${FWAppKit}
)

if(MACOS)
    # Resources:
    set(qmake_immediate_resource_files
        "util/FocusFrame.qml"
    )

    qt_internal_add_resource(qtquickcontrols2nativestyleplugin "qmake_immediate"
        PREFIX
            "/"
        FILES
            ${qmake_immediate_resource_files}
    )
endif()

qt_internal_extend_target(qtquickcontrols2nativestyleplugin CONDITION WIN32
    SOURCES
        qstyle/windows/qquickwindowsstyle.cpp qstyle/windows/qquickwindowsstyle_p.h
        qstyle/windows/qquickwindowsstyle_p_p.h
        qstyle/windows/qquickwindowsxpstyle.cpp
    INCLUDE_DIRECTORIES
        qstyle/windows
    LIBRARIES
        gdi32
        user32
        uxtheme
)

set(qml_files
    "controls/DefaultButton.qml"
    "controls/DefaultSlider.qml"
    "controls/DefaultGroupBox.qml"
    "controls/DefaultCheckBox.qml"
    "controls/DefaultRadioButton.qml"
    "controls/DefaultSpinBox.qml"
    "controls/DefaultTextField.qml"
    "controls/DefaultFrame.qml"
    "controls/DefaultTextArea.qml"
    "controls/DefaultComboBox.qml"
    "controls/DefaultScrollBar.qml"
    "controls/DefaultProgressBar.qml"
    "controls/DefaultDial.qml"
)
set_source_files_properties(controls/DefaultButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultSlider.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultGroupBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultCheckBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultRadioButton.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultSpinBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultTextField.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultFrame.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultTextArea.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultComboBox.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultScrollBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultProgressBar.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)
set_source_files_properties(controls/DefaultDial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "2.0;6.0"
)

qt6_target_qml_files(qtquickcontrols2nativestyleplugin
    FILES
        ${qml_files}
)
