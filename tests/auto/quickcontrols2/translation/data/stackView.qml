// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    id: window
    width: 640
    height: 480

    property var engine

    signal calledTranslate

    property int pagesCreated: 0

    Component {
        id: translatePageComponent
        Item {
            id: root

            Component.onCompleted: objectName = "translatePageRootItem" + pagesCreated++

            StackView.onActivating: {
                retranslateTimer.start()
            }

            Timer {
                id: retranslateTimer
                objectName: "retranslateTimer"
                interval: 100
                repeat: true

                property int count: 0

                onTriggered: {

                    // This triggers the crash without retranslate().
                    // retranslate() got cleverer in 6.2, sidestepping the issue.
                    replaceExit.animations[0].target = null;
                    replaceExit.animations[0].target = replaceExit.ViewTransition.item

                    console.log("timer within", root, "is about to call retranslate")
                    window.engine.retranslate()
                    window.calledTranslate()
                    count++
                    if(count >= 10) {
                        stop()
                        count = 0
                    }
                }
            }

            Button {
                objectName: "button"
                text: qsTr("Push")
                onClicked: root.StackView.view.replace(translatePageComponent)
            }
        }
    }

    StackView {
        id: stack
        objectName: "stackView"
        anchors.fill: parent
        initialItem: translatePageComponent

        replaceEnter: Transition {
            id: replaceEnter
            objectName: "enterTransition"

            PropertyAnimation {
                objectName: replaceEnter.objectName + "PropertyAnimation"
                target: replaceEnter.ViewTransition.item
                property: "opacity"
                from: 0
                to: 1
                duration: 10
            }

        }

        replaceExit: Transition {
            id: replaceExit
            objectName: "exitTransition"

            PropertyAnimation {
                objectName: replaceExit.objectName + "PropertyAnimation"
                target: replaceExit.ViewTransition.item
                property: "opacity"
                from: 1
                to: 0
                duration: 10
            }
        }
    }
}

