// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 130
    height: 200
    property int widthFactor: 7

    Column {
        anchors.fill: parent
        Text {
            id: text1
            width: parent.width;
            text: qsTrId("TRID_1")
        }
        Text {
            id: text2
            width: 100;
            text: qsTrId("TRID_2")
        }
        Text {
            id: text3
            width: parent.width;
            text: qsTrId("TRID_3")
        }
        Text {
            id: text4
            width: parent.width;
            text: qsTrId("TRID_4")
        }
        Text {
            id: text5
            width: parent.width;
            text: qsTrId("TRID_5")
        }
        Text {
            id: text6
            width: parent.width;
            text: "way too long not translated text that should elide but not be marked"
        }
    }

    states: [
        State {
            name: "BiggerFontState"

            PropertyChanges {
                target: text1
                font.pointSize: 20
            }

            PropertyChanges {
                target: text2
                font.pointSize: 20
            }

            PropertyChanges {
                target: text3
                font.pointSize: 20
            }

        },
        State {
            name: "WayBiggerFontState"

            PropertyChanges {
                target: text1
                font.pointSize: 30
            }

            PropertyChanges {
                target: text2
                font.pointSize: 30
            }

            PropertyChanges {
                target: text3
                font.pointSize: 30
            }
        }
    ]
}
