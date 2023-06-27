// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//![0]
Column {
    Text {
        font.pointSize: 24
        text: "<b>Hello</b> <i>World!</i>"
    }
    Text {
        font.pointSize: 24
        textFormat: Text.RichText
        text: "<b>Hello</b> <i>World!</i>"
    }
    Text {
        font.pointSize: 24
        textFormat: Text.PlainText
        text: "<b>Hello</b> <i>World!</i>"
    }
    Text {
        font.pointSize: 24
        textFormat: Text.MarkdownText
        text: "**Hello** *World!*"
    }
}
//![0]

