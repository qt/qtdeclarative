// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Column {
    x: 10
    y: 10
    width: 240
    height: 240
    spacing: 24

//! [text]
    Text {
        textFormat: Text.StyledText
        text: qsTr('Hover <a href="https://qt.io" title="A cross-platform framework">Qt</a> ' +
                   'or <img title="The Qt Company" src="logo.png"/>')

        ToolTip.text: hoveredToolTip
        ToolTip.visible: hoveredToolTip !== ""
    }
//! [text]

//! [textedit]
    TextEdit {
        textFormat: TextEdit.MarkdownText
        text: qsTr('Hover [Qt](https://qt.io "A cross-platform framework") or ' +
                   '![logo](logo.png "The Qt Company")')

        ToolTip.text: hoveredToolTip
        ToolTip.visible: hoveredToolTip !== ""
    }
//! [textedit]

//! [textedit-follow]
    TextEdit {
        id: textEdit
        textFormat: TextEdit.RichText
        text: qsTr('Hover <a href="https://qt.io" title="A cross-platform framework">Qt</a> ' +
                   'or <img title="The Qt Company" src="logo.png"/>')

        HoverHandler {
            id: hoverHandler
        }
        ToolTip {
            visible: textEdit.hoveredToolTip.length > 0
            x: hoverHandler.point.position.x + 10
            y: hoverHandler.point.position.y + 10
            text: textEdit.hoveredToolTip
        }
    }
//! [textedit-follow]
}
