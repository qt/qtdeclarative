// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    id: page

    property real ratio: width / 320 < height / 440 ? width / 320 : height / 440
    property int elementSpacing: 6.3 * ratio

    width: 320
    height: 440

    Button {
      anchors.top: parent.top
      anchors.right: parent.right
      anchors.margins: 10
      text: hidingRect.visible ? qsTr("Hide") : qsTr("Show")
      onClicked: hidingRect.visible = !hidingRect.visible
    }

    // Create column with four rectangles, the fourth one is hidden
    Column {
        id: column

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: page.width / 32
        anchors.topMargin: page.height / 48
        spacing: page.elementSpacing

        //! [0]
        Rectangle {
            id: green
            color: "#80c342"
            width: 100 * page.ratio
            height: 100 * page.ratio

            Text {
              anchors.left: parent.right
              anchors.leftMargin: 20
              anchors.verticalCenter: parent.verticalCenter
              text: qsTr("Index: %1%2%3").arg(parent.Positioner.index)
                        .arg(parent.Positioner.isFirstItem ? qsTr(" (First)") : "")
                        .arg(parent.Positioner.isLastItem ? qsTr(" (Last)") : "")
            }

            // When mouse is clicked, display the values of the positioner
            MouseArea {
                anchors.fill: parent
                onClicked: column.showInfo(green.Positioner)
            }
        }
        //! [0]

        Rectangle {
            id: blue
            color: "#14aaff"
            width: 100 * page.ratio
            height: 100 * page.ratio

            Text {
              anchors.left: parent.right
              anchors.leftMargin: 20
              anchors.verticalCenter: parent.verticalCenter
              text: qsTr("Index: %1%2%3").arg(parent.Positioner.index)
                        .arg(parent.Positioner.isFirstItem ? qsTr(" (First)") : "")
                        .arg(parent.Positioner.isLastItem ? qsTr(" (Last)") : "")
            }

            // When mouse is clicked, display the values of the positioner
            MouseArea {
                anchors.fill: parent
                onClicked: column.showInfo(blue.Positioner)
            }
        }

        Rectangle {
            id: purple
            color: "#6400aa"
            width: 100 * page.ratio
            height: 100 * page.ratio

            Text {
              anchors.left: parent.right
              anchors.leftMargin: 20
              anchors.verticalCenter: parent.verticalCenter
              text: qsTr("Index: %1%2%3").arg(parent.Positioner.index)
                        .arg(parent.Positioner.isFirstItem ? qsTr(" (First)") : "")
                        .arg(parent.Positioner.isLastItem ? qsTr(" (Last)") : "")
            }

            // When mouse is clicked, display the values of the positioner
            MouseArea {
                anchors.fill: parent
                onClicked: column.showInfo(purple.Positioner)
            }
        }

        // This rectangle is not visible, so it doesn't have a positioner value
        Rectangle {
            id: hidingRect
            color: "#006325"
            width: 100 * page.ratio
            height: 100 * page.ratio
            visible: false

            Text {
                anchors.left: parent.right
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Index: %1%2%3").arg(parent.Positioner.index)
                          .arg(parent.Positioner.isFirstItem ? qsTr(" (First)") : "")
                          .arg(parent.Positioner.isLastItem ? qsTr(" (Last)") : "")
            }
        }

        // Print the index of the child item in the positioner and convenience
        // properties showing if it's the first or last item.
        function showInfo(positioner) {
            console.log(qsTr("Item Index = ") + positioner.index)
            console.log(qsTr("  isFirstItem = ") + positioner.isFirstItem)
            console.log(qsTr("  isLastItem = ") + positioner.isLastItem)
        }
    }
}
