// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    clip: true

    property alias text: textItem.text
    property bool highlight: false
    required property bool current
    required property bool selected
    required property bool editing
    required property string edit

    signal commit(text: string)

    readonly property bool __darkMode: Qt.styleHints.colorScheme === Qt.Dark
    border {
        width: (!editing && current) ? 1 : 0
        color: current ? palette.highlight.darker(__darkMode ? 0.7 : 1.9) : palette.base
    }
    readonly property color __highlight_color: __darkMode
                                               ? palette.highlight.darker(1.9)
                                               : palette.highlight.lighter(1.9)
    color: highlight ? __highlight_color : selected ? palette.highlight : palette.base

    Label {
        id: textItem
        anchors { fill: parent; margins: 5 }
        visible: !root.editing
    }

    TableView.editDelegate: TextField {
        anchors.fill: root
        text: root.edit
        TableView.onCommit: root.commit(text)
    }
}
