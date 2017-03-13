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
import "WatchFace"
import "Fitness"
import "Navigation"
import "Style"

Item {
    Item {
        anchors.centerIn: parent

        width: UIStyle.visibleDiameter
        height: UIStyle.visibleDiameter

        ListModel {
            id: viewModel

            ListElement { // 0
                title: qsTr("World Clock")
                icon: "../images/watchface/watch.png"
                page: "WatchFace/WatchFaceMain.qml"
            }
            ListElement { // 1
                title: qsTr("Navigation")
                icon: "../images/navigation/route.png"
                page: "Navigation/NavigationMain.qml"
            }
            ListElement { // 2
                title: qsTr("Weather")
                icon: "../images/weather/weather.png"
                page: "Weather/WeatherMain.qml"
            }
            ListElement { // 3
                title: qsTr("Fitness")
                icon: "../images/fitness/fitness.png"
                page: "Fitness/FitnessMain.qml"
            }
            ListElement { // 4
                title: qsTr("Notifications")
                icon: "../images/notifications/notifications.png"
                page: "Notifications/NotificationsMain.qml"
            }
            ListElement { // 5
                title: qsTr("Alarm")
                icon: "../images/alarms/alarms.png"
                page: "Alarms/AlarmsMain.qml"
            }
            ListElement { // 6
                title: qsTr("Settings")
                icon: "../images/settings/settings.png"
                page: "Settings/SettingsMain.qml"
            }
        }

        PathView {
            id: circularView
            property int objSize: 60
            property int cX: parent.width / 2
            property int cY: parent.height / 2

            currentIndex: 0

            anchors.fill: parent
            model: viewModel
            delegate: pathDelegate
            snapMode: PathView.SnapToItem

            path: Path {
                startX: circularView.cX
                startY: circularView.cY
                PathAttribute {
                    name: "itemOpacity"
                    value: 1.0
                }
                PathLine {
                    x: circularView.width - circularView.objSize
                    y: circularView.cY
                }
                PathAttribute {
                    name: "itemOpacity"
                    value: 0.7
                }
                PathArc {
                    x: circularView.objSize
                    y: circularView.cY
                    radiusX: circularView.cX - circularView.objSize
                    radiusY: circularView.cY - circularView.objSize
                    useLargeArc: true
                    direction: PathArc.Clockwise
                }
                PathAttribute {
                    name: "itemOpacity"
                    value: 0.5
                }
                PathArc {
                    x: circularView.width - circularView.objSize
                    y: circularView.cY
                    radiusX: circularView.cX - circularView.objSize
                    radiusY: circularView.cY - circularView.objSize
                    useLargeArc: true
                    direction: PathArc.Clockwise
                }
                PathAttribute {
                    name: "itemOpacity"
                    value: 0.3
                }
            }
        }

        Component {
            id: pathDelegate

            Item {
                id: wrapper

                width: childrenRect.width
                height: childrenRect.height

                Column {
                    opacity: wrapper.PathView.itemOpacity
                    Item {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: wrapper.PathView.view.objSize
                        width: wrapper.PathView.view.objSize

                        Image {
                            anchors.fill: parent
                            source: icon
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "transparent"

                            border.width: 3
                            border.color: wrapper.PathView.isCurrentItem ?
                                            "transparent"
                                            : UIStyle.colorQtGray4
                        }
                    }

                    Text {
                        id: appTitle

                        opacity: wrapper.PathView.isCurrentItem
                                    && (wrapper.PathView.itemOpacity === 1.0)

                        anchors.horizontalCenter: parent.horizontalCenter
                        font.bold: true
                        font.pixelSize: UIStyle.fontSizeS
                        font.letterSpacing: 1
                        color: UIStyle.colorQtGray1
                        text: title
                    }
                }

                MouseArea {
                    id: pathItemMouseArea
                    anchors.fill: parent
                    onClicked: {
                        if (circularView.currentIndex === index)
                            loadPage()
                        else
                            circularView.currentIndex = index
                    }
                }
                function loadPage() {
                    stackView.push(Qt.resolvedUrl(page))
                }
            }
        }
    }
}
