// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property alias labelText: label.text
    property string toolTipText
    property alias color: textField.text

    ToolTip.text: toolTipText
    ToolTip.visible: textField.hovered

    Layout.columnSpan: 2

    Label {
        id: label

        Layout.fillWidth: true
    }

    TextField {
        id: textField

        validator: RegularExpressionValidator{
            regularExpression: /#?[0-9a-fA-F]*/
        }

        Layout.preferredWidth: 200
        Layout.fillWidth: false
    }
}
