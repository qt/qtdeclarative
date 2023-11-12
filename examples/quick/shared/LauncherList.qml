// Copyright (C) 2017 Crimson AS <info@crimson.no>
// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQml.Models
import QtQuick

Rectangle {
    id: root
    property int activePageCount: 0

    //model is a list of {"name":"somename", "url":"file:///some/url/mainfile.qml"}
    //function used to add to model A) to enforce scheme B) to allow Qt.resolveUrl in url assignments

    color: "#eee"
    function addExample(name, desc, url) {
        myModel.append({"name":name, "description":desc, "url":url})
    }
    function showExample(url) {
        pageComponent.createObject(pageContainer, { exampleUrl: url }).show()
    }

    // The container rectangle here is used to give a nice "feel" when
    // transitioning into an example.
    Rectangle {
        anchors.fill: parent
        color: "black"

        ListView {
            id: launcherList
            clip: true
            delegate: SimpleLauncherDelegate{
                required property url url
                onClicked: root.showExample(url)
            }
            model: ListModel {id:myModel}
            anchors.fill: parent
            enabled: opacity == 1.0
        }
    }

    Item {
        id: pageContainer
        anchors.fill: parent
    }

    Component {
        id: pageComponent
        Rectangle {
            id: page
            clip: true
            property url exampleUrl
            width: parent.width
            height: parent.height - bar.height
            color: "white"
            TapHandler {
                //Eats mouse events
            }
            Loader{
                focus: true
                source: parent.exampleUrl
                anchors.fill: parent
            }

            function show() {
                showAnim.start()
            }

            function exit() {
                exitAnim.start()
            }

            ParallelAnimation {
                id: showAnim
                ScriptAction {
                    script: root.activePageCount++
                }
                NumberAnimation {
                    target: launcherList
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 500
                }
                NumberAnimation {
                    target: launcherList
                    property: "scale"
                    from: 1.0
                    to: 0.0
                    duration: 500
                }
                NumberAnimation {
                    target: page
                    property: "x"
                    from: -page.width
                    to: 0
                    duration: 300
                }
            }
            SequentialAnimation {
                id: exitAnim

                ScriptAction {
                    script: root.activePageCount--
                }

                ParallelAnimation {
                    NumberAnimation {
                        target: launcherList
                        property: "opacity"
                        from: 0.0
                        to: 1.0
                        duration: 300
                    }
                    NumberAnimation {
                        target: launcherList
                        property: "scale"
                        from: 0.0
                        to: 1.0
                        duration: 300
                    }
                    NumberAnimation {
                        target: page
                        property: "x"
                        from: 0
                        to: -page.width
                        duration: 300
                    }
                }

                ScriptAction {
                    script: page.destroy()
                }
            }
        }
    }
    Rectangle {
        id: bar
        visible: height > 0
        anchors.bottom: parent.bottom
        width: parent.width
        height: root.activePageCount > 0 ? 40 : 0

        Behavior on height {
            NumberAnimation {
                duration: 300
            }
        }

        Rectangle {
            height: 1
            color: "#ccc"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
        }

        Rectangle {
            height: 1
            color: "#fff"
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
        }

        gradient: Gradient {
            GradientStop { position: 0 ; color: "#eee" }
            GradientStop { position: 1 ; color: "#ccc" }
        }

        Image {
            id: back
            source: "images/back.png"
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 1
            anchors.left: parent.left
            anchors.leftMargin: 6
            width: 38
            height: 31
            fillMode: Image.Pad
            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignVCenter

            TapHandler {
                id: tapHandler
                enabled: root.activePageCount > 0
                gesturePolicy: TapHandler.ReleaseWithinBounds
                longPressThreshold: 0
                onTapped: {
                    pageContainer.children[pageContainer.children.length - 1].exit()
                }
            }
            Rectangle {
                anchors.fill: parent
                opacity: tapHandler.pressed ? 1 : 0
                Behavior on opacity { NumberAnimation{ duration: 100 }}
                gradient: Gradient {
                    GradientStop { position: 0 ; color: "#22000000" }
                    GradientStop { position: 0.2 ; color: "#11000000" }
                }
                border.color: "darkgray"
                antialiasing: true
                radius: 4
            }
        }
    }
}
