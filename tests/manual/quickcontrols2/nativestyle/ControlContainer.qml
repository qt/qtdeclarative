// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.rightMargin: 7
    implicitHeight: groupBox.height

    property alias title: groupBox.title
    property real rowSpacing: 20

    default property alias data: layout.data

    GroupBox {
        id: groupBox
        anchors.left: parent.left
        anchors.right: parent.right

        ColumnLayout {
            id: layout
            spacing: 15
        }
    }
}
