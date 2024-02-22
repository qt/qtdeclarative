// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 320
    height: 240
    property int tapCount: 0

    Flickable {
        width: (parent.width / 2) - 20
        height: parent.height - 20
        x: 10; y: 10
        contentWidth: 300
        contentHeight: 300
        clip: true
        Rectangle {
            width: 140
            height: 220
            border.color: "black"
            color: "aquamarine"
            TextEdit {
                id: textEdit
                objectName: "textEdit"
                anchors.fill: parent
                anchors.margins: 2
                text:  "No! those days are gone away,
And their hours are old and gray,
And their minutes buried all
Under the down-trodden pall
Of the leaves of many years:
Many times have winter's shears,
Frozen North, and chilling East,
Sounded tempests to the feast
Of the forest's whispering fleeces,
Since men knew nor rent nor leases."
            }
        }
    }
}
