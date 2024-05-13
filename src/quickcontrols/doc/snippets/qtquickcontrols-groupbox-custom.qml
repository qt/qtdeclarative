// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

GroupBox {
    id: control
    title: qsTr("GroupBox")

    background: Rectangle {
        y: control.topPadding - control.bottomPadding
        width: parent.width
        height: parent.height - control.topPadding + control.bottomPadding
        color: "transparent"
        border.color: "#21be2b"
        radius: 2
    }

    label: Label {
        x: control.leftPadding
        width: control.availableWidth
        text: control.title
        color: "#21be2b"
        elide: Text.ElideRight
    }

    Label {
        text: qsTr("Content goes here!")
    }
}
//! [file]
