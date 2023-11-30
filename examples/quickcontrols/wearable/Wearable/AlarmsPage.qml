// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle

Item {
    function addAlarm() {
        alarms.append(
            {
               "title": qsTr("New alarm"),
               "hour": 0,
               "minute": 0,
               "nextRing": qsTr("Thursday 20. Nov."),
               "armed": false
            })
    }

    QQC2.AbstractButton {
        id: plusButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 5
        anchors.leftMargin: 5

        width: height
        height: 40
        onClicked: parent.addAlarm()

        Image {
            anchors.centerIn: parent
            width: 32
            height: 32
            source: UIStyle.iconPath("plus")
        }
    }

    ListModel {
        id: alarms
        ListElement {
            title: qsTr("Workday morning")
            hour: 6
            minute: 30
            nextRing: qsTr("Thursday 20. Nov.")
            armed: false
        }
        ListElement {
            title: qsTr("Weekend morning")
            hour: 8
            minute: 40
            nextRing: qsTr("Saturday 22. Nov.")
            armed: false
        }
    }

    property alias alarmList: listview

    ListView {
        id: listview

        anchors.fill: parent
        anchors.margins: 15
        anchors.topMargin: 40 + 15
        spacing: 10

        model: alarms

        delegate: ListHeaderItem {
            id: alarmItem

            property alias onSwitch: onSwitch
            property alias minuteTumbler: minuteTumbler
            property alias hourTumbler: hourTumbler

            required property string title
            required property string nextRing
            required property bool armed

            width: parent.width
            height: 94

            Item {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 28

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 10
                    text: alarmItem.title
                    color: UIStyle.titletextColor
                    font: UIStyle.h3
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.margins: 10
                    text: alarmItem.nextRing
                    color: UIStyle.titletextColor
                    font: UIStyle.p1
                }
            }

            Item {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 66

                Switch {
                    id: onSwitch
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 15
                    checked: alarmItem.armed
                }

                QQC2.Tumbler {
                    id: minuteTumbler
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    visibleItemCount: 1
                    model: 60

                    delegate: Text {
                        required property int modelData
                        text: (modelData < 10 ? "0" : "") + modelData
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 5
                        font: UIStyle.tumblerFont
                        color: UIStyle.textColor
                    }
                }

                Text {
                    id: timespacer
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: minuteTumbler.left
                    verticalAlignment: Text.AlignVCenter
                    font: UIStyle.tumblerFont
                    color: UIStyle.textColor
                    text: ":"
                }

                QQC2.Tumbler {
                    id: hourTumbler
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: timespacer.left
                    visibleItemCount: 1
                    model: 12

                    delegate: Text {
                        required property int modelData
                        text: (modelData < 10 ? "0" : "") + modelData
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        rightPadding: 5
                        font: UIStyle.tumblerFont
                        color: UIStyle.textColor
                    }
                }
            }
        }
    }
}
