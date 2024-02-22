// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: layout.implicitWidth
    height: layout.implicitHeight

    property int num: 6
    property double rStep: 2
    GridLayout {
        id: layout
        columns: num*num
        rows: num*num
        columnSpacing: 0
        rowSpacing: 0
        Repeater {
            model: num*num*num*num
            Rectangle {
                //transform: Rotation {angle: 10} works too
                color: Qt.rgba(1, 0, 0, 1)
                border.color: Qt.rgba(0, 0, 0, 1)
                border.width: 4
                topLeftRadius: rStep*Math.floor(index/num/num/num)
                topRightRadius: rStep*(Math.floor(index/num/num)%num)
                bottomLeftRadius: rStep*(Math.floor(index/num)%num)
                bottomRightRadius: rStep*(index%num)
                antialiasing: true
                opacity: 0.4

                Layout.preferredWidth: 2*(num-1)*rStep
                Layout.preferredHeight: 2*(num-1)*rStep
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 0
            }
        }
    }
}
