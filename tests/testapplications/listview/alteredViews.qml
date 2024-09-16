// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 300
    height: 400

    ListView {
        id: listview
        model: model1
        delegate: delegate1
        anchors.fill: parent
        anchors.margins: 20
    }

    Component {
        id: delegate1
        Rectangle {
            height: listview.orientation == ListView.Horizontal ? 260 : 50
            Behavior on height { NumberAnimation { duration: 500 } }
            width: listview.orientation == ListView.Horizontal ? 50 : 260
            Behavior on width { NumberAnimation { duration: 500 } }
            border.color: "black"
            Text {
                anchors.centerIn: parent; color: "black"; text: model.name
                rotation: listview.orientation == ListView.Horizontal ? -90 : 0
                Behavior on rotation { NumberAnimation { duration: 500 } }

            }
        }
    }

    Component {
        id: delegate2
        Rectangle {
            height: listview.orientation == ListView.Horizontal ? 260 : 50
            Behavior on height { NumberAnimation { duration: 500 } }
            width: listview.orientation == ListView.Horizontal ? 50 : 260
            Behavior on width { NumberAnimation { duration: 500 } }
            color: "goldenrod"; border.color: "black"
            Text {
                anchors.centerIn: parent; color: "royalblue"; text: model.name
                rotation: listview.orientation == ListView.Horizontal ? -90 : 0
                Behavior on rotation { NumberAnimation { duration: 1500 } }
            }
        }

    }

    Column {
        Rectangle {
            height: 50
            width: 50
            color: "blue"
            border.color: "orange"
            Text {
                anchors.centerIn: parent
                text: "Mod"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: listview.model = listview.model == model2 ? model1 : model2
            }
        }

        Rectangle {
            height: 50
            width: 50
            color: "blue"
            border.color: "orange"
            Text {
                anchors.centerIn: parent
                text: "Del"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: listview.delegate = listview.delegate == delegate2 ? delegate1 : delegate2
            }
        }

        Rectangle {
            height: 50
            width: 50
            color: "blue"
            border.color: "orange"
            Text {
                anchors.centerIn: parent
                text: "Ori"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: listview.orientation = listview.orientation == ListView.Horizontal ? ListView.Vertical : ListView.Horizontal
            }
        }
    }

    ListModel {
        id: model1
        ListElement { name: "model1_1" }
        ListElement { name: "model1_2" }
        ListElement { name: "model1_3" }
        ListElement { name: "model1_4" }
        ListElement { name: "model1_5" }
    }

    ListModel {
        id: model2
        ListElement { name: "model2_1" }
        ListElement { name: "model2_2" }
        ListElement { name: "model2_3" }
        ListElement { name: "model2_4" }
        ListElement { name: "model2_5" }
    }
}
