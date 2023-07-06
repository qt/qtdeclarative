// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

PathView {
    id: circularView

    signal launched(string page)

    readonly property int cX: width / 2
    readonly property int cY: height / 2
    readonly property int itemSize: size / 4
    readonly property int size: Math.min(width - 80, height)
    readonly property int radius: size / 2 - itemSize / 3

    snapMode: PathView.SnapToItem

    model: ListModel {
        ListElement {
            title: qsTr("World Clock")
            icon: "worldclock"
            page: "WorldClockPage.qml"
        }
        ListElement {
            title: qsTr("Navigation")
            icon: "navigation"
            page: "NavigationPage.qml"
        }
        ListElement {
            title: qsTr("Weather")
            icon: "weather"
            page: "WeatherPage.qml"
        }
        ListElement {
            title: qsTr("Fitness")
            icon: "fitness"
            page: "FitnessPage.qml"
        }
        ListElement {
            title: qsTr("Notifications")
            icon: "notifications"
            page: "NotificationsPage.qml"
        }
        ListElement {
            title: qsTr("Alarm")
            icon: "alarms"
            page: "AlarmsPage.qml"
        }
        ListElement {
            title: qsTr("Settings")
            icon: "settings"
            page: "SettingsPage.qml"
        }
    }

    delegate: QQC2.RoundButton {
        width: circularView.itemSize
        height: circularView.itemSize

        property string title: model.title

        icon.width: 36
        icon.height: 36
        icon.name: model.icon
        icon.color: UIStyle.colorQtGray1
        opacity: PathView.itemOpacity
        padding: 12

        background: Rectangle {
            radius: width / 2
            color: UIStyle.colorQtGray10
            border.width: 3
            border.color: parent.PathView.isCurrentItem ? UIStyle.colorQtPrimGreen : UIStyle.themeColorQtGray4
        }

        onClicked: {
            if (PathView.isCurrentItem)
                circularView.launched(Qt.resolvedUrl(page))
            else
                circularView.currentIndex = index
        }
    }

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
        anchors.verticalCenterOffset: (circularView.itemSize + height) / 2

        font.bold: true
        font.pixelSize: circularView.itemSize / 3
        font.letterSpacing: 1
        color: UIStyle.themeColorQtGray1
    }
}
