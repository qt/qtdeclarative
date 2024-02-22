// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "."

ListView {
    id: root
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    Layout.fillWidth: true
    Layout.preferredHeight: count > 0 ? 128 : noneLabel.implicitHeight

    ScrollBar.vertical: ScrollBar {}

    delegate: TextField {
        width: root.width
        text: modelData
        readOnly: true
        selectByMouse: true
    }

    Label {
        id: noneLabel
        text: qsTr("(None)")
        visible: root.count === 0
    }
}
