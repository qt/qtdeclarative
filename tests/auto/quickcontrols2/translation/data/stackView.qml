/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

