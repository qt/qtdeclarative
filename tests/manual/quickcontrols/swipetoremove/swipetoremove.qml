// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 300
    height: 400
    visible: true
    title: qsTr("Swipe to Remove")

    ListView {
        id: listView
        anchors.fill: parent

        delegate: SwipeDelegate {
            id: delegate

            text: modelData
            width: listView.width

            //! [delegate]
            swipe.right: Rectangle {
                width: parent.width
                height: parent.height

                clip: true
                color: SwipeDelegate.pressed ? "#555" : "#666"

                Label {
                    font.family: "Fontello"
                    text: delegate.swipe.complete ? "\ue805" // icon-cw-circled
                                                  : "\ue801" // icon-cancel-circled-1

                    padding: 20
                    anchors.fill: parent
                    horizontalAlignment: Qt.AlignRight
                    verticalAlignment: Qt.AlignVCenter

                    opacity: 2 * -delegate.swipe.position

                    color: Material.color(delegate.swipe.complete ? Material.Green : Material.Red, Material.Shade200)
                    Behavior on color { ColorAnimation { } }
                }

                Label {
                    text: qsTr("Removed")
                    color: "white"

                    padding: 20
                    anchors.fill: parent
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter

                    opacity: delegate.swipe.complete ? 1 : 0
                    Behavior on opacity { NumberAnimation { } }
                }

                SwipeDelegate.onClicked: delegate.swipe.close()
                SwipeDelegate.onPressedChanged: undoTimer.stop()
            }
            //! [delegate]

            //! [removal]
            Timer {
                id: undoTimer
                interval: 3600
                onTriggered: listModel.remove(index)
            }

            swipe.onCompleted: undoTimer.start()
            //! [removal]
        }

        model: ListModel {
            id: listModel
            ListElement { text: "Lorem ipsum dolor sit amet" }
            ListElement { text: "Curabitur sit amet risus" }
            ListElement { text: "Suspendisse vehicula nisi" }
            ListElement { text: "Mauris imperdiet libero" }
            ListElement { text: "Sed vitae dui aliquet augue" }
            ListElement { text: "Praesent in elit eu nulla" }
            ListElement { text: "Etiam vitae magna" }
            ListElement { text: "Pellentesque eget elit euismod" }
            ListElement { text: "Nulla at enim porta" }
            ListElement { text: "Fusce tincidunt odio" }
            ListElement { text: "Ut non ex a ligula molestie" }
            ListElement { text: "Nam vitae justo scelerisque" }
            ListElement { text: "Vestibulum pulvinar tellus" }
            ListElement { text: "Quisque dignissim leo sed gravida" }
        }

        //! [transitions]
        remove: Transition {
            SequentialAnimation {
                PauseAnimation { duration: 125 }
                NumberAnimation { property: "height"; to: 0; easing.type: Easing.InOutQuad }
            }
        }

        displaced: Transition {
            SequentialAnimation {
                PauseAnimation { duration: 125 }
                NumberAnimation { property: "y"; easing.type: Easing.InOutQuad }
            }
        }
        //! [transitions]

        ScrollIndicator.vertical: ScrollIndicator { }
    }

    Label {
        id: placeholder
        text: qsTr("Swipe no more")

        anchors.margins: 60
        anchors.fill: parent

        opacity: 0.5
        visible: listView.count === 0

        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        wrapMode: Label.WordWrap
        font.pixelSize: 18
    }
}
