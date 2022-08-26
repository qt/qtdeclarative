# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

file(GLOB_RECURSE __aux_qml_glob_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "images/*.png"
)
qt_path_join(images_destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick/Controls/designer/images")
qt_copy_or_install(
    FILES
        ${__aux_qml_glob_files}
    DESTINATION "${images_destination}"
)
qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick/Controls/designer")
qt_copy_or_install(
    FILES
        AbstractButtonSection.qml
        BusyIndicatorSpecifics.qml
        ButtonSection.qml
        ButtonSpecifics.qml
        CheckBoxSpecifics.qml
        CheckDelegateSpecifics.qml
        CheckSection.qml
        ComboBoxSpecifics.qml
        ContainerSection.qml
        ControlSection.qml
        ControlSpecifics.qml
        DelayButtonSpecifics.qml
        DialSpecifics.qml
        FrameSpecifics.qml
        GroupBoxSpecifics.qml
        InsetSection.qml
        ItemDelegateSection.qml
        ItemDelegateSpecifics.qml
        LabelSpecifics.qml
        PaddingSection.qml
        PageIndicatorSpecifics.qml
        PageSpecifics.qml
        PaneSection.qml
        PaneSpecifics.qml
        ProgressBarSpecifics.qml
        RadioButtonSpecifics.qml
        RadioDelegateSpecifics.qml
        RangeSliderSpecifics.qml
        RoundButtonSpecifics.qml
        ScrollViewSpecifics.qml
        SliderSpecifics.qml
        SpinBoxSpecifics.qml
        StackViewSpecifics.qml
        SwipeDelegateSpecifics.qml
        SwipeViewSpecifics.qml
        SwitchDelegateSpecifics.qml
        SwitchSpecifics.qml
        TabBarSpecifics.qml
        TabButtonSpecifics.qml
        TextAreaSpecifics.qml
        TextFieldSpecifics.qml
        ToolBarSpecifics.qml
        ToolButtonSpecifics.qml
        ToolSeparatorSpecifics.qml
        TumblerSpecifics.qml
        qtquickcontrols2.metainfo
    DESTINATION "${destination}"
)
