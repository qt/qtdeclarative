# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## StyleKit Module:
#####################################################################

qt_internal_add_qml_module(QtQuickStyleKit
    URI Qt.labs.StyleKit
    IMPORTS QtQuick.Controls.Basic
    SOURCES
        qqstylekit_p.h qqstylekit.cpp
        qqstylekitcontrol_p.h qqstylekitcontrol.cpp
        qqstylekitcontrols_p.h qqstylekitcontrols.cpp
        qqstylekitcontrolstate_p.h qqstylekitcontrolstate.cpp
        qqstylekitcontrolproperties_p.h qqstylekitcontrolproperties.cpp
        qqstylekitcustomcontrol_p.h qqstylekitcustomcontrol.cpp
        qqstylekitdebug_p.h qqstylekitdebug.cpp
        qqstylekitdelegate_p.h qqstylekitdelegate.cpp
        qqstylekitglobal_p.h qqstylekitglobal.cpp
        qqstylekitpalette_p.h qqstylekitpalette.cpp
        qqstylekitfont_p.h qqstylekitfont.cpp
        qqstylekitpropertyresolver_p.h qqstylekitpropertyresolver.cpp
        qqstylekitreader_p.h qqstylekitreader.cpp
        qqstylekitstyle_p.h qqstylekitstyle.cpp
        qqstylekittheme_p.h qqstylekittheme.cpp
        qqstylekitstyleandthemebase_p.h qqstylekitstyleandthemebase.cpp
        qqstylekitcustomtheme_p.h qqstylekitcustomtheme.cpp
        qqstylekitvariation_p.h qqstylekitvariation.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    QML_FILES
        ApplicationWindow.qml
        Button.qml
        CheckBox.qml
        ComboBox.qml
        Frame.qml
        ItemDelegate.qml
        ProgressBar.qml
        RadioButton.qml
        RangeSlider.qml
        ScrollBar.qml
        ScrollIndicator.qml
        ScrollView.qml
        Slider.qml
        Style.qml
        StyleKitAnimation.qml
        StyleKitDelegate.qml
        SpinBox.qml
        Switch.qml
        TextField.qml
        TabBar.qml
        TabButton.qml
        ToolBar.qml
        ToolButton.qml
        ToolSeparator.qml
        Page.qml
        Pane.qml
        Popup.qml
        Label.qml
        GroupBox.qml
        TextArea.qml
    LIBRARIES
        Qt6::GuiPrivate
        Qt6::Quick
        Qt6::QuickPrivate
        Qt6::QuickTemplates2
        Qt6::QuickTemplates2Private
        Qt6::QuickControls2ImplPrivate
)

# impl links to QuickStyleKit, so it must be built after it
add_subdirectory(impl)
