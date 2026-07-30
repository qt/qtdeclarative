# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## StyleKit Module:
#####################################################################

qt_internal_add_qml_module(LabsStyleKit
    URI Qt.labs.StyleKit
    VERSION "${PROJECT_VERSION}"
    DEPENDENCIES
        QtQuick/auto
    IMPORTS
        QtQuick.Controls.Basic/auto
    SOURCES
        qqstylekit_p.h qqstylekit.cpp
        qqstylekitanimation.cpp
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
        CheckDelegate.qml
        ComboBox.qml
        Dialog.qml
        DialogButtonBox.qml
        Frame.qml
        ItemDelegate.qml
        Menu.qml
        MenuBar.qml
        MenuBarItem.qml
        MenuItem.qml
        MenuSeparator.qml
        PageIndicator.qml
        ProgressBar.qml
        RadioButton.qml
        RadioDelegate.qml
        RoundButton.qml
        RangeSlider.qml
        ScrollBar.qml
        ScrollIndicator.qml
        ScrollView.qml
        SearchField.qml
        Slider.qml
        Style.qml
        StyleAnimation.qml
        SpinBox.qml
        SwipeDelegate.qml
        Switch.qml
        SwitchDelegate.qml
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
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickPrivate
        Qt6::QuickTemplates2
        Qt6::QuickTemplates2Private
        Qt6::QuickControls2ImplPrivate
)

qt_internal_add_docs(LabsStyleKit
    doc/qtlabsstylekit.qdocconf
)

# impl links to QuickStyleKit, so it must be built after it
add_subdirectory(impl)

if(TARGET Qt::Widgets)
    qt_internal_extend_target(LabsStyleKit
        SOURCES
            widgets/qstylekitstyle.h widgets/qstylekitstyle_p.h widgets/qstylekitstyle.cpp
        PUBLIC_LIBRARIES
            Qt::Widgets
        LIBRARIES
            Qt::WidgetsPrivate
            Qt::Qml
            Qt::QmlPrivate
    )
    # The style plugin (widgets/plugin) is intentionally not built
    # during TP phase. As a widget style plugin it would be discoverable
    # by every Qt Widgets application through QStyleFactory::keys() and
    # also cause deployment bloat by pulling in QtQuick/QML stack into widget
    # apps that didn't opt in. Use QStyleKitStyle directly instead.
    # add_subdirectory(widgets/plugin)
endif()
