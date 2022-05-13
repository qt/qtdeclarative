// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Pane {
    padding: 0

    property var delegateComponentMap: {
        "ItemDelegate": itemDelegateComponent,
        "SwipeDelegate": swipeDelegateComponent,
        "CheckDelegate": checkDelegateComponent,
        "RadioDelegate": radioDelegateComponent,
        "SwitchDelegate": switchDelegateComponent
    }

    Component {
        id: itemDelegateComponent

        ItemDelegate {
            text: labelText
            width: parent.width
        }
    }

    Component {
        id: swipeDelegateComponent

        SwipeDelegate {
            id: swipeDelegate
            text: labelText
            width: parent.width

            Component {
                id: removeComponent

                Rectangle {
                    color: SwipeDelegate.pressed ? "#333" : "#444"
                    width: parent.width
                    height: parent.height
                    clip: true

                    SwipeDelegate.onClicked: view.model.remove(ourIndex)

                    Label {
                        font.pixelSize: swipeDelegate.font.pixelSize
                        text: "Remove"
                        color: "white"
                        anchors.centerIn: parent
                    }
                }
            }

            swipe.left: removeComponent
            swipe.right: removeComponent
        }
    }

    Component {
        id: checkDelegateComponent

        CheckDelegate {
            text: labelText
        }
    }

    ButtonGroup {
        id: radioButtonGroup
    }

    Component {
        id: radioDelegateComponent

        RadioDelegate {
            text: labelText
            ButtonGroup.group: radioButtonGroup
        }
    }

    Component {
        id: switchDelegateComponent

        SwitchDelegate {
            text: labelText
        }
    }

    ColumnLayout {
        id: column
        spacing: 40
        anchors.fill: parent
        anchors.topMargin: 20

        Label {
            Layout.fillWidth: true
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: "Delegate controls are used as delegates in views such as ListView."
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ListModel {
                ListElement { type: "ItemDelegate"; text: "ItemDelegate" }
                ListElement { type: "ItemDelegate"; text: "ItemDelegate" }
                ListElement { type: "ItemDelegate"; text: "ItemDelegate" }
                ListElement { type: "SwipeDelegate"; text: "SwipeDelegate" }
                ListElement { type: "SwipeDelegate"; text: "SwipeDelegate" }
                ListElement { type: "SwipeDelegate"; text: "SwipeDelegate" }
                ListElement { type: "CheckDelegate"; text: "CheckDelegate" }
                ListElement { type: "CheckDelegate"; text: "CheckDelegate" }
                ListElement { type: "CheckDelegate"; text: "CheckDelegate" }
                ListElement { type: "RadioDelegate"; text: "RadioDelegate" }
                ListElement { type: "RadioDelegate"; text: "RadioDelegate" }
                ListElement { type: "RadioDelegate"; text: "RadioDelegate" }
                ListElement { type: "SwitchDelegate"; text: "SwitchDelegate" }
                ListElement { type: "SwitchDelegate"; text: "SwitchDelegate" }
                ListElement { type: "SwitchDelegate"; text: "SwitchDelegate" }
            }

            section.property: "type"
            section.delegate: Pane {
                width: listView.width
                height: sectionLabel.implicitHeight + 20

                Label {
                    id: sectionLabel
                    text: section
                    anchors.centerIn: parent
                }
            }

            delegate: Loader {
                id: delegateLoader
                width: listView.width
                sourceComponent: delegateComponentMap[text]

                property string labelText: text
                property ListView view: listView
                property int ourIndex: index

                // Can't find a way to do this in the SwipeDelegate component itself,
                // so do it here instead.
                SequentialAnimation {
                    id: removeAnimation

                    PropertyAction {
                        target: delegateLoader
                        property: "ListView.delayRemove"
                        value: true
                    }
                    NumberAnimation {
                        target: item
                        property: "height"
                        to: 0
                        easing.type: Easing.InOutQuad
                    }
                    PropertyAction {
                        target: delegateLoader
                        property: "ListView.delayRemove"
                        value: false
                    }
                }
                ListView.onRemove: removeAnimation.start()
            }
        }
    }
}
