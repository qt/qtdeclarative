// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 200
    height: 200

    property alias listView: listView

    palette.highlight: "red"

    ListView {
        id: listView
        anchors.fill: parent
        model: 1
        delegate: Column {
            property alias control: control
            property alias label: label
            property alias textarea: textarea
            property alias textfield: textfield

            Control { id: control }
            Label { id: label }
            TextArea { id: textarea }
            TextField { id: textfield }
        }
    }
}
