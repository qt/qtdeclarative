// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    implicitWidth: parent.width
    implicitHeight: 180
    title: "Usage"

    ScrollView {
        anchors.fill: parent
        contentWidth: textArea.implicitWidth
        TextArea {
            id: textArea
            readonly property int slashIndex: bridge.targetDirectory.lastIndexOf("/")
            readonly property string importPath: bridge.targetDirectory.slice(0, slashIndex)
            readonly property string styleName: bridge.targetDirectory.slice(slashIndex + 1)
            readOnly: true
            wrapMode: TextEdit.Wrap
            textFormat: TextEdit.RichText
            text: slashIndex === -1 || importPath === "" || styleName === ""
                  ? "You first need to set a valid target directory!"
                  : bridge.howToText()
            .replace(/@styleName@/g, styleName)
            .replace(/@importPath@/g, importPath)
        }
    }
}
