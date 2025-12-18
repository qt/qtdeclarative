// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultComboBox {
    id: control
    readonly property Item __focusFrameTarget: control

    background: NativeStyle.ComboBox {
        control: control
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
        useNinePatchImage: false

        readonly property bool __ignoreNotCustomizable: true
    }

    contentItem: T.TextField {
        implicitWidth: contentWidth
        implicitHeight: contentHeight
        text: control.editable ? control.editText : control.displayText

        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse

        color: control.editable ? control.palette.text : control.palette.buttonText
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        verticalAlignment: Text.AlignVCenter

        readonly property Item __focusFrameControl: control
        readonly property bool __ignoreNotCustomizable: true
    }
}
