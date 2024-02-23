// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [0]
TextEdit {
    id: edit
    width: 300
    height: 200
    textFormat: TextEdit.MarkdownText
    textDocument.source: "example.md"
    wrapMode: TextEdit.WordWrap

    Text {
        anchors {
            bottom: parent.bottom
            right: parent.right
        }
        color: edit.textDocument.status === TextDocument.Loaded ? "darkolivegreen" : "tomato"
        text:
            switch (edit.textDocument.status) {
            case TextDocument.Loading:
                return qsTr("Loading ") + edit.textDocument.source
            case TextDocument.Loaded:
                return qsTr("Loaded ") + edit.textDocument.source
            default:
                return edit.textDocument.errorString
            }
    }
}
//! [0]
