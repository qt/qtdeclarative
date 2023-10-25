// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

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
            text: qsTr("Delegate controls are used as delegates in views such as ListView.")
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

            model: ListModel {
                ListElement { type: "ItemDelegate"; value: qsTr("ItemDelegate1") }
                ListElement { type: "ItemDelegate"; value: qsTr("ItemDelegate2") }
                ListElement { type: "ItemDelegate"; value: qsTr("ItemDelegate3") }
                ListElement { type: "SwipeDelegate"; value: qsTr("SwipeDelegate1") }
                ListElement { type: "SwipeDelegate"; value: qsTr("SwipeDelegate2") }
                ListElement { type: "SwipeDelegate"; value: qsTr("SwipeDelegate3") }
                ListElement { type: "CheckDelegate"; value: qsTr("CheckDelegate1") }
                ListElement { type: "CheckDelegate"; value: qsTr("CheckDelegate2") }
                ListElement { type: "CheckDelegate"; value: qsTr("CheckDelegate3") }
                ListElement { type: "RadioDelegate"; value: qsTr("RadioDelegate1") }
                ListElement { type: "RadioDelegate"; value: qsTr("RadioDelegate2") }
                ListElement { type: "RadioDelegate"; value: qsTr("RadioDelegate3") }
                ListElement { type: "SwitchDelegate"; value: qsTr("SwitchDelegate1") }
                ListElement { type: "SwitchDelegate"; value: qsTr("SwitchDelegate2") }
                ListElement { type: "SwitchDelegate"; value: qsTr("SwitchDelegate3") }
            }

            delegate: Loader {
                id: delegateLoader
                width: ListView.view.width
                sourceComponent: delegateComponentMap[type]

                required property string value
                required property string type
                required property var model
                required property int index

                property ListView view: listView

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
                        text: delegateLoader.value
                        width: delegateLoader.width
                    }
                }

                Component {
                    id: swipeDelegateComponent

                    SwipeDelegate {
                        id: swipeDelegate
                        text: delegateLoader.value
                        width: delegateLoader.width

                        Component {
                            id: removeComponent

                            Rectangle {
                                color: SwipeDelegate.pressed ? "#333" : "#444"
                                width: parent.width
                                height: parent.height
                                clip: true

                                SwipeDelegate.onClicked: {
                                    if (delegateLoader.view !== undefined)
                                        delegateLoader.view.model.remove(delegateLoader.index)
                                }

                                Label {
                                    font.pixelSize: swipeDelegate.font.pixelSize
                                    text: qsTr("Remove")
                                    color: "white"
                                    anchors.centerIn: parent
                                }
                            }
                        }

                        SequentialAnimation {
                            id: removeAnimation

                            PropertyAction {
                                target: delegateLoader
                                property: "ListView.delayRemove"
                                value: true
                            }
                            NumberAnimation {
                                target: swipeDelegate
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

                        swipe.left: removeComponent
                        swipe.right: removeComponent
                        ListView.onRemove: removeAnimation.start()
                    }
                }

                Component {
                    id: checkDelegateComponent

                    CheckDelegate {
                        text: delegateLoader.value
                    }
                }

                ButtonGroup {
                    id: radioButtonGroup
                }

                Component {
                    id: radioDelegateComponent

                    RadioDelegate {
                        text: delegateLoader.value

                        ButtonGroup.group: radioButtonGroup
                    }
                }

                Component {
                    id: switchDelegateComponent

                    SwitchDelegate {
                        text: delegateLoader.value
                    }
                }
            }
        }
    }
}
