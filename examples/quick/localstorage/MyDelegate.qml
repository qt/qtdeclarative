// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: delegate

    required property int index
    required property int distance
    required property string trip_desc
    required property string date

    signal clicked()

    width: ListView.view.width
    implicitHeight: rDate.implicitHeight * 1.5

    Rectangle {
        id: baseRec
        anchors.fill: parent
        opacity: 0.8
        color: delegate.index % 2 ? "lightgrey" : "grey"
        border {
            width: 2
            color: Qt.lighter(color)
        }
        radius: 5

        MouseArea {
            anchors.fill: parent
            onClicked: delegate.clicked()
        }

        RowLayout {
            anchors.fill: parent

            Label {
                id: rDate
                Layout.preferredWidth: 42
                Layout.alignment: Qt.AlignCenter
                horizontalAlignment: Text.AlignHCenter
                text: delegate.date
                font.pixelSize: 22
                color: "black"
            }

            Label {
                Layout.preferredWidth: 42
                Layout.alignment: Qt.AlignCenter
                horizontalAlignment: Text.AlignHCenter
                text: delegate.trip_desc
                font.pixelSize: 22
                color: "black"
            }

            Label {
                Layout.preferredWidth: 42
                Layout.alignment: Qt.AlignCenter
                horizontalAlignment: Text.AlignHCenter
                text: delegate.distance
                font.pixelSize: 22
                color: "black"
            }
        }
    }
}
