// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import Wearable
import WearableStyle
import "fitness.js" as FitnessData

Item {
    QQC2.SwipeView {
        id: svFitnessContainer

        anchors.fill: parent

        SwipeViewPage {
            id: fitnessPage1

            Column {
                anchors.centerIn: parent
                spacing: 15

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Steps: ") + FitnessData.getSteps()
                    font.italic: true
                    font.pixelSize: UIStyle.fontSizeM
                    color: UIStyle.themeColorQtGray1
                }
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: UIStyle.themeImagePath("fitness-man-walking")
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Calories: ") + FitnessData.getCalories()
                    font.pixelSize: UIStyle.fontSizeS
                    font.italic: true
                    color: UIStyle.themeColorQtGray3
                }
            }
        }

        SwipeViewPage {
            id: fitnessPage2

            Column {
                anchors.centerIn: parent
                spacing: 15

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Distance: ")
                          + FitnessData.getDistance()
                          + qsTr(" miles")
                    font.italic: true
                    font.pixelSize: UIStyle.fontSizeM
                    color: UIStyle.themeColorQtGray1
                }
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: UIStyle.themeImagePath("fitness-man-running")
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Time: ")
                          + FitnessData.getTime()
                          + qsTr(" mins")
                    font.pixelSize: UIStyle.fontSizeS
                    font.italic: true
                    color: UIStyle.themeColorQtGray3
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svFitnessContainer.count
        currentIndex: svFitnessContainer.currentIndex

        anchors.bottom: svFitnessContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
