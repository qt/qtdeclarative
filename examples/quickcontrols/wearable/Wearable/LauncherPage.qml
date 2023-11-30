// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

//! [LauncherPage start]
PathView {
    id: circularView

    signal launched(string title, string page, string fallbackpage)
//! [LauncherPage start]

    readonly property int cX: width / 2
    readonly property int cY: height / 2
    readonly property int size: Math.min(width - 80, height)
    readonly property int itemSize: size / 5
    readonly property int radius: size / 2 - itemSize / 4

    snapMode: PathView.SnapToItem

//! [Model start]
    model: ListModel {
//! [Model start]
        ListElement {
            title: qsTr("World Clock")
            pageIcon: "clock"
            page: "WorldClockPage.qml"
            fallback: ""
        }
//! [Model mid]
        ListElement {
            title: qsTr("Navigation")
            pageIcon: "maps"
            page: "NavigationPage.qml"
            fallback: "NavigationFallbackPage.qml"
        }
//! [Model mid]
        ListElement {
            title: qsTr("Weather")
            pageIcon: "weather"
            page: "WeatherPage.qml"
            fallback: "WeatherPage.qml"
        }
        ListElement {
            title: qsTr("Fitness")
            pageIcon: "hearth"
            page: "FitnessPage.qml"
            fallback: ""
        }
        ListElement {
            title: qsTr("Notifications")
            pageIcon: "notification"
            page: "NotificationsPage.qml"
            fallback: ""
        }
        ListElement {
            title: qsTr("Alarm")
            pageIcon: "bell"
            page: "AlarmsPage.qml"
            fallback: ""
        }
//! [Model end]
        ListElement {
            title: qsTr("Settings")
            pageIcon: "settings"
            page: "SettingsPage.qml"
            fallback: ""
        }
    }
//! [Model end]

//! [Delegate start]
    delegate: QQC2.RoundButton {
//! [Delegate start]
        width: circularView.itemSize
        height: circularView.itemSize

        required property string title
        required property string pageIcon
        required property string page
        required property string fallback
        required property int index

//! [Delegate mid]
        icon.width: 36
        icon.height: 36
        icon.source: UIStyle.iconPath(pageIcon)
        icon.color: UIStyle.textColor
//! [Delegate mid]
        padding: 12

        background: Rectangle {
            radius: width / 2
            color: parent.PathView.isCurrentItem ? UIStyle.highlightColor : UIStyle.buttonBackground
            border.width: 1
            border.color: UIStyle.buttonGrayOutLine

            Rectangle {
                radius: parent.radius
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: UIStyle.gradientOverlay1
                    }
                    GradientStop {
                        position: 1.0
                        color: UIStyle.gradientOverlay2
                    }
                }
            }
        }

//! [Delegate end]
        onClicked: {
            if (PathView.isCurrentItem)
                circularView.launched(title, Qt.resolvedUrl(page), Qt.resolvedUrl(fallback))
            else
                circularView.currentIndex = index
        }
    }
//! [Delegate end]

    path: Path {
        startX: circularView.cX
        startY: circularView.cY
        PathAttribute {
            name: "itemOpacity"
            value: 1.0
        }
        PathLine {
            x: circularView.cX + circularView.radius
            y: circularView.cY
        }
        PathAttribute {
            name: "itemOpacity"
            value: 0.7
        }
        PathArc {
            x: circularView.cX - circularView.radius
            y: circularView.cY
            radiusX: circularView.radius
            radiusY: circularView.radius
            useLargeArc: true
            direction: PathArc.Clockwise
        }
        PathAttribute {
            name: "itemOpacity"
            value: 0.5
        }
        PathArc {
            x: circularView.cX + circularView.radius
            y: circularView.cY
            radiusX: circularView.radius
            radiusY: circularView.radius
            useLargeArc: true
            direction: PathArc.Clockwise
        }
        PathAttribute {
            name: "itemOpacity"
            value: 0.3
        }
    }

    Text {
        id: appTitle

        property Item currentItem: circularView.currentItem

        visible: currentItem ? currentItem.PathView.itemOpacity === 1.0 : 0

        text: currentItem ? currentItem.title : ""
        anchors.centerIn: parent
        anchors.verticalCenterOffset: circularView.itemSize / 2 + height / 2

        font: UIStyle.h1
        color: UIStyle.textColor
    }
//! [LauncherPage end]
}
//! [LauncherPage end]
