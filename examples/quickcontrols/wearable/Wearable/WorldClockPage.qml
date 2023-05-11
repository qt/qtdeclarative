// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

Item {
    QQC2.SwipeView {
        id: svWatchContainer

        anchors.fill: parent

        ListModel {
            id: placesList
            ListElement {
                cityName: "New York"
                timeShift: -4
            }
            ListElement {
                cityName: "London"
                timeShift: 0
            }
            ListElement {
                cityName: "Oslo"
                timeShift: 1
            }
            ListElement {
                cityName: "Mumbai"
                timeShift: 5.5
            }
            ListElement {
                cityName: "Tokyo"
                timeShift: 9
            }
            ListElement {
                cityName: "Brisbane"
                timeShift: 10
            }
            ListElement {
                cityName: "Los Angeles"
                timeShift: -8
            }
        }

        Repeater {
            model: placesList
            delegate: Clock {
            }
        }
    }

    QQC2.PageIndicator {
        count: svWatchContainer.count
        currentIndex: svWatchContainer.currentIndex

        anchors.bottom: svWatchContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
