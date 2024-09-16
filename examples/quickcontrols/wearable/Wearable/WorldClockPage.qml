// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2

Item {
    QQC2.SwipeView {
        id: svWatchContainer

        anchors.fill: parent

        ListModel {
            id: placesList
            ListElement {
                cityName: "New York"
                shift: -4
            }
            ListElement {
                cityName: "London"
                shift: 0
            }
            ListElement {
                cityName: "Oslo"
                shift: 1
            }
            ListElement {
                cityName: "Mumbai"
                shift: 5.5
            }
            ListElement {
                cityName: "Tokyo"
                shift: 9
            }
            ListElement {
                cityName: "Brisbane"
                shift: 10
            }
            ListElement {
                cityName: "Los Angeles"
                shift: -8
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
