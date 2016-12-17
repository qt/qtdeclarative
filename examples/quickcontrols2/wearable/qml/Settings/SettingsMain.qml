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

PageContainer {
    Rectangle {
        anchors.centerIn: parent

        width: UIStyle.visibleDiameter
        height: UIStyle.visibleRectHeight

        color: "transparent"

        SettingsData {
            id: settingsData
        }

        QQC2.SwipeView {
            id: svSettingsContainer

            anchors.fill: parent

            clip: true
            currentIndex: 0

            Item {
                id: settingsPage1

                Column {
                    anchors.centerIn: parent
                    spacing: 25

                    Row {
                        spacing: 50
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            height: 64
                            width: 64
                            source: "../../images/settings/bluetooth.png"
                        }
                        WSwitch {
                            id: bluetoothSwitch
                            anchors.verticalCenter: parent.verticalCenter
                            checked: settingsData.bluetooth.state
                        }
                    }
                    Row {
                        spacing: 50
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            height: 64
                            width: 64
                            source: "../../images/settings/wifi.png"
                        }
                        WSwitch {
                            id: wirelessSwitch
                            anchors.verticalCenter: parent.verticalCenter
                            checked: settingsData.wireless.state
                        }
                    }
                }
            }

            Item {
                id: settingsPage2

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Column {
                        Image {
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: 64
                            width: 64
                            source: "../../images/settings/brightness.png"
                        }
                        WSlider {
                            id: brightnessSlider
                            anchors.horizontalCenter: parent.horizontalCenter
                            value: settingsData.brightness.value
                            from: settingsData.brightness.min
                            to: settingsData.brightness.max
                            stepSize: settingsData.brightness.steps
                        }
                    }
                    Column {
                        spacing: 2
                        Image {
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: 64
                            width: 64
                            source: "../../images/settings/contrast.png"
                        }
                        WSlider {
                            id: contrastSlider
                            anchors.horizontalCenter: parent.horizontalCenter
                            value: settingsData.contrast.value
                            from: settingsData.contrast.min
                            to: settingsData.contrast.max
                            stepSize: settingsData.contrast.steps
                        }
                    }
                }
            }
        }

        QQC2.PageIndicator {
            id: pgSettingsIndicator

            anchors.bottom: svSettingsContainer.bottom
            anchors.bottomMargin: 1
            anchors.horizontalCenter: parent.horizontalCenter

            count: svSettingsContainer.count
            currentIndex: svSettingsContainer.currentIndex

            delegate: Rectangle {
                implicitWidth: 8
                implicitHeight: 8

                radius: width / 2
                color: UIStyle.colorQtGray3

                opacity: index === pgSettingsIndicator.currentIndex ? 1.0 : 0.35

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 100
                    }
                }
            }
        }
    }
}
