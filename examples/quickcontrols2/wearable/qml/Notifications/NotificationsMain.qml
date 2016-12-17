/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick 2.7
import QtQuick.Controls 2.0 as QQC2
import "../Common"
import "notifications.js" as NotificationData

PageContainer {
    Rectangle {
        anchors.centerIn: parent

        width: UIStyle.visibleDiameter
        height: UIStyle.visibleRectHeight

        color: "transparent"

        QQC2.SwipeView {
            id: svNotificationsContainer

            anchors.fill: parent

            clip: true
            currentIndex: 0

            Item {
                id: notificationsPage1

                ListModel {
                    id: missedCallsList
                }

                Row {
                    anchors.fill: parent
                    leftPadding: 30
                    spacing: 2

                    Image {
                        id: missedCallIcon
                        anchors.verticalCenter: parent.verticalCenter
                        height: 64
                        width: 64
                        source: "../../images/notifications/missedcall.png"
                    }

                    ListView {
                        id: missedCallsView
                        width: parent.width - missedCallIcon.width
                        height: parent.height

                        clip: true
                        focus: true
                        boundsBehavior: Flickable.StopAtBounds
                        snapMode: ListView.SnapToItem

                        model: missedCallsList

                        delegate: Rectangle {
                            radius: 10
                            color: "transparent"
                            height: missedCallsView.height
                            width: missedCallsView.width
                            Column {
                                anchors.fill: parent
                                spacing: 15
                                topPadding: 35
                                Image {
                                    anchors.horizontalCenter:
                                     parent.horizontalCenter
                                    height: 64
                                    width: 64
                                    source: (gender == "m") ?
                                    "../../images/notifications/avatarm.png"
                                    :"../../images/notifications/avatarf.png"
                                }

                                Text {
                                    anchors.horizontalCenter:
                                     parent.horizontalCenter
                                    text: name
                                    font.bold: true
                                    font.pixelSize: UIStyle.fontSizeS
                                    color: UIStyle.colorQtGray1
                                }
                                Text {
                                    anchors.horizontalCenter:
                                     parent.horizontalCenter
                                    text: date + " " + time
                                    font.pixelSize: UIStyle.fontSizeXS
                                    font.italic: true
                                    color: UIStyle.colorQtGray2
                                }
                            }
                        }
                    }
                }
            }
        }

        QQC2.PageIndicator {
            id: pgNotificationsIndicator

            anchors.bottom: svNotificationsContainer.bottom
            anchors.bottomMargin: 1
            anchors.horizontalCenter: parent.horizontalCenter

            count: svNotificationsContainer.count
            currentIndex: svNotificationsContainer.currentIndex

            delegate: Rectangle {
                implicitWidth: 8
                implicitHeight: 8

                radius: width / 2
                color: UIStyle.colorQtGray3

                opacity: index === pgNotificationsIndicator.currentIndex
                         ? 1.0 : 0.35

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 100
                    }
                }
            }
        }

        Component.onCompleted: {
            NotificationData.populateData(missedCallsList)
        }
    }
}
