// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Pane {
    ColumnLayout {
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
            clip: true
            section.property: "type"
            section.delegate: Pane {
                id: sectionPane
                required property string section
                width: ListView.view.width
                height: sectionLabel.implicitHeight + 20
                Label {
                    id: sectionLabel
                    text: sectionPane.section
                    anchors.centerIn: parent
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property var delegateComponentMap: {
                "ItemDelegate": itemDelegateComponent,
                "SwipeDelegate": swipeDelegateComponent,
                "CheckDelegate": checkDelegateComponent,
                "RadioDelegate": radioDelegateComponent,
                "SwitchDelegate": switchDelegateComponent
            }

            Component {
                id: itemDelegateComponent

                ItemDelegate {
                    // qmllint disable unqualified
                    text: value
                    // qmllint enable unqualified
                    width: parent.width
                }
            }

            Component {
                id: swipeDelegateComponent

                SwipeDelegate {
                    id: swipeDelegate
                    // qmllint disable unqualified
                    text: value
                    // qmllint enable unqualified
                    width: parent.width

                    Component {
                        id: removeComponent

                        Rectangle {
                            color: SwipeDelegate.pressed ? "#333" : "#444"
                            width: parent.width
                            height: parent.height
                            clip: true

                            SwipeDelegate.onClicked: {
                                // qmllint disable unqualified
                                view.model.remove(ourIndex)
                                // qmllint enable unqualified
                            }

                            Label {
                                // qmllint disable unqualified
                                font.pixelSize: swipeDelegate.font.pixelSize
                                // qmllint enable unqualified
                                text: "Remove"
                                color: "white"
                                anchors.centerIn: parent
                            }
                        }
                    }

                    SequentialAnimation {
                        id: removeAnimation

                        PropertyAction {
                            // qmllint disable unqualified
                            target: delegateItem
                            // qmllint enable unqualified
                            property: "ListView.delayRemove"
                            value: true
                        }
                        NumberAnimation {
                            // qmllint disable unqualified
                            target: delegateItem.item
                            // qmllint enable unqualified
                            property: "height"
                            to: 0
                            easing.type: Easing.InOutQuad
                        }
                        PropertyAction {
                            // qmllint disable unqualified
                            target: delegateItem
                            // qmllint enable unqualified
                            property: "ListView.delayRemove"
                            value: false
                        }
                    }

                    swipe.left: removeComponent
                    swipe.right: removeComponent
                    ListView.onRemove: removeAnimation.start()
                }
            }

            Component {
                id: checkDelegateComponent

                CheckDelegate {
                    // qmllint disable unqualified
                    text: value
                    // qmllint enable unqualified
                }
            }

            ButtonGroup {
                id: radioButtonGroup
            }

            Component {
                id: radioDelegateComponent

                RadioDelegate {
                    // qmllint disable unqualified
                    text: value
                    ButtonGroup.group: radioButtonGroup
                    // qmllint enable unqualified
                }
            }

            Component {
                id: switchDelegateComponent

                SwitchDelegate {
                    // qmllint disable unqualified
                    text: value
                    // qmllint enable unqualified
                }
            }

            model: ListModel {
                ListElement { type: "ItemDelegate"; value: "ItemDelegate1" }
                ListElement { type: "ItemDelegate"; value: "ItemDelegate2" }
                ListElement { type: "ItemDelegate"; value: "ItemDelegate3" }
                ListElement { type: "SwipeDelegate"; value: "SwipeDelegate1" }
                ListElement { type: "SwipeDelegate"; value: "SwipeDelegate2" }
                ListElement { type: "SwipeDelegate"; value: "SwipeDelegate3" }
                ListElement { type: "CheckDelegate"; value: "CheckDelegate1" }
                ListElement { type: "CheckDelegate"; value: "CheckDelegate2" }
                ListElement { type: "CheckDelegate"; value: "CheckDelegate3" }
                ListElement { type: "RadioDelegate"; value: "RadioDelegate1" }
                ListElement { type: "RadioDelegate"; value: "RadioDelegate2" }
                ListElement { type: "RadioDelegate"; value: "RadioDelegate3" }
                ListElement { type: "SwitchDelegate"; value: "SwitchDelegate1" }
                ListElement { type: "SwitchDelegate"; value: "SwitchDelegate2" }
                ListElement { type: "SwitchDelegate"; value: "SwitchDelegate3" }
            }

            delegate: Loader {
                id: delegateLoader
                width: ListView.view.width
                // qmllint disable unqualified
                sourceComponent: listView.delegateComponentMap[type]
                // qmllint enable unqualified

                required property string value
                required property string type
                required property var model
                required property int index

                property Loader delegateItem: delegateLoader
                // qmllint disable unqualified
                property ListView view: listView
                // qmllint enable unqualified
                property int ourIndex: index
            }
        }
    }
}
