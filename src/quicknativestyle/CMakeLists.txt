# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2nativestyleplugin Plugin:
#####################################################################

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

if(MACOS)
    list(APPEND qml_files "util/FocusFrame.qml")
endif()

if(QT_FEATURE_quick_treeview)
    list(APPEND qml_files "controls/DefaultTreeViewDelegate.qml")
endif()

qt_internal_add_qml_module(qtquickcontrols2nativestyleplugin
    URI "QtQuick.NativeStyle"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2NativeStylePlugin
    DEPENDENCIES
        QtQuick.Controls/auto
        QtQuick.Layouts/auto
        QtQuick/auto
    PAST_MAJOR_VERSIONS 2
    PLUGIN_TARGET qtquickcontrols2nativestyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        items/qquickstyleitem.cpp items/qquickstyleitem.h
        items/qquickstyleitembutton.cpp items/qquickstyleitembutton.h
        items/qquickstyleitemcheckbox.cpp items/qquickstyleitemcheckbox.h
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
    QML_FILES
        ${qml_files}
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
)

qt_internal_extend_target(qtquickcontrols2nativestyleplugin CONDITION QT_FEATURE_quick_treeview
    SOURCES
        items/qquickstyleitemtreeindicator.cpp items/qquickstyleitemtreeindicator.h
)

qt_internal_extend_target(qtquickcontrols2nativestyleplugin CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        items/qquickstyleitemcombobox.cpp items/qquickstyleitemcombobox.h
)

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

qt_internal_extend_target(qtquickcontrols2nativestyleplugin CONDITION WIN32
    SOURCES
        qstyle/windows/qquickwindowsstyle.cpp qstyle/windows/qquickwindowsstyle_p.h
        qstyle/windows/qquickwindowsstyle_p_p.h
        qstyle/windows/qquickwindowsxpstyle.cpp
        qstyle/windows/qquickwindowsxpstyle_p.h
        qstyle/windows/qquickwindowsxpstyle_p_p.h
    INCLUDE_DIRECTORIES
        qstyle/windows
    LIBRARIES
        gdi32
        user32
        uxtheme
)
